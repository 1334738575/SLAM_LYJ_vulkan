#include "VulkanORBMatcher.h"

#include "VulkanConfig.h"
#include "VulkanDefines.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

NSP_VULKAN_LYJ_BEGIN

namespace {

struct alignas(16) ORBMatcherParams
{
	int kpSz1 = 0;
	int kpSz2 = 0;
	int wGrid = 0;
	int hGrid = 0;

	int gridResolution = VKORBGRIDSOLU;
	int distThDesc = 0;
	int use3D = 0;
	int cameraModel = 0;

	float nnTh = 0.0f;
	float squareDistTh3D = 0.0f;
	float fx = 0.0f;
	float fy = 0.0f;

	float cx = 0.0f;
	float cy = 0.0f;
	float width = 0.0f;
	float height = 0.0f;

	float Twc1Rows[12]{};
	float Twc2Rows[12]{};
	float Tcw2Rows[12]{};
	float fundamentalRows[12]{};
	float distortion[4]{};
};

static_assert(sizeof(ORBMatcherParams) == 272, "ORB matcher UBO layout must match GLSL std140 layout");

template <typename T>
void destroyPtr(std::shared_ptr<T>& ptr)
{
	if (ptr) {
		ptr->destroy();
		ptr.reset();
	}
}

bool ensureVulkan()
{
	auto* instance = GetLYJVKInstance();
	if (instance->isInited())
		return true;
	return instance->init(false, nullptr, false) == VK_SUCCESS;
}

VkQueue getComputeQueue(unsigned int queueIndex)
{
	auto* instance = GetLYJVKInstance();
	VkQueue queue = instance->getComputeQueue(static_cast<int>(queueIndex));
	if (queue == VK_NULL_HANDLE)
		queue = instance->getComputeQueue(0);
	return queue;
}

template <typename T>
void uploadPadded(VKBufferAbr& buffer, const T* data, size_t count, VkQueue queue, VkFence fence = VK_NULL_HANDLE)
{
	if (count == 0)
		return;
	const size_t byteSize = count * sizeof(T);
	const size_t paddedSize = (byteSize + 63) / 64 * 64;
	std::vector<unsigned char> upload(paddedSize, 0);
	std::memcpy(upload.data(), data, byteSize);
	buffer.upload(static_cast<VkDeviceSize>(paddedSize), upload.data(), queue, fence);
}

template <typename T>
void uploadPadded(VKBufferAbr& buffer, const std::vector<T>& data, VkQueue queue, VkFence fence = VK_NULL_HANDLE)
{
	uploadPadded(buffer, data.data(), data.size(), queue, fence);
}

void setIdentity(std::array<float, 12>& transform)
{
	transform.fill(0.0f);
	transform[0] = 1.0f;
	transform[4] = 1.0f;
	transform[8] = 1.0f;
}

void copyTransform(std::array<float, 12>& destination, const float* source)
{
	if (source)
		std::copy(source, source + 12, destination.begin());
}

void makeTransformRows(float* rows, const std::array<float, 12>& transform)
{
	rows[0] = transform[0];
	rows[1] = transform[3];
	rows[2] = transform[6];
	rows[3] = transform[9];
	rows[4] = transform[1];
	rows[5] = transform[4];
	rows[6] = transform[7];
	rows[7] = transform[10];
	rows[8] = transform[2];
	rows[9] = transform[5];
	rows[10] = transform[8];
	rows[11] = transform[11];
}

void computeFundamentalRows(float* rows, const std::array<float, 12>& Tcw2,
	const std::array<float, 12>& Twc1, const float* camera)
{
	const float R1[9] = {
		Twc1[0], Twc1[3], Twc1[6],
		Twc1[1], Twc1[4], Twc1[7],
		Twc1[2], Twc1[5], Twc1[8]
	};
	const float R2[9] = {
		Tcw2[0], Tcw2[3], Tcw2[6],
		Tcw2[1], Tcw2[4], Tcw2[7],
		Tcw2[2], Tcw2[5], Tcw2[8]
	};
	float R[9]{};
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			for (int k = 0; k < 3; ++k)
				R[r * 3 + c] += R2[r * 3 + k] * R1[k * 3 + c];

	const float t1[3] = { Twc1[9], Twc1[10], Twc1[11] };
	const float t2[3] = { Tcw2[9], Tcw2[10], Tcw2[11] };
	float t[3]{};
	for (int r = 0; r < 3; ++r)
		t[r] = R2[r * 3] * t1[0] + R2[r * 3 + 1] * t1[1] + R2[r * 3 + 2] * t1[2] + t2[r];

	const float tx[9] = {
		0.0f, -t[2], t[1],
		t[2], 0.0f, -t[0],
		-t[1], t[0], 0.0f
	};
	float E[9]{};
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			for (int k = 0; k < 3; ++k)
				E[r * 3 + c] += tx[r * 3 + k] * R[k * 3 + c];

	const float invFx = 1.0f / camera[0];
	const float invFy = 1.0f / camera[1];
	const float invK[9] = {
		invFx, 0.0f, -camera[2] * invFx,
		0.0f, invFy, -camera[3] * invFy,
		0.0f, 0.0f, 1.0f
	};
	float temp[9]{};
	float F[9]{};
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			for (int k = 0; k < 3; ++k)
				temp[r * 3 + c] += E[r * 3 + k] * invK[k * 3 + c];
	for (int r = 0; r < 3; ++r)
		for (int c = 0; c < 3; ++c)
			for (int k = 0; k < 3; ++k)
				F[r * 3 + c] += invK[k * 3 + r] * temp[k * 3 + c];
	for (int r = 0; r < 3; ++r) {
		rows[r * 4] = F[r * 3];
		rows[r * 4 + 1] = F[r * 3 + 1];
		rows[r * 4 + 2] = F[r * 3 + 2];
		rows[r * 4 + 3] = 0.0f;
	}
}

void initializeComputeBuffer(std::shared_ptr<VKBufferCompute>& buffer, VkDeviceSize size)
{
	buffer.reset(new VKBufferCompute());
	buffer->resize(size);
}

void waitFence(VKFence& fence)
{
	fence.wait();
	fence.reset();
}

} // namespace

class ORBMatcherVK
{
public:
	ORBMatcherVK(int width, int height, const float* camera, CameraModel cameraModel)
		: width_(width), height_(height), cameraModel_(cameraModel)
	{
		if (camera) {
			if (width <= 0 || height <= 0)
				throw std::invalid_argument("invalid ORB matcher image size");
			if (cameraModel_ == CameraModel::Fisheye)
				std::copy(camera, camera + 8, camera_.begin());
			else
				std::copy(camera, camera + 4, camera_.begin());
			hasCamera_ = true;
		}
		if (!ensureVulkan())
			throw std::runtime_error("failed to initialize Vulkan for ORB matcher");
	}

	bool matchBF(ORBMatcherCacheVK& cache, int distThDesc, float nnTh, char use3D, float squareDistTh3D)
	{
		if (!validCommon(cache, distThDesc, nnTh, use3D, true))
			return false;
		buildBF(cache);
		return run(cache, cache.matchBFImp_, makeParams(cache, distThDesc, nnTh, use3D, squareDistTh3D));
	}

	bool matchF(ORBMatcherCacheVK& cache, int distThDesc, float nnTh, char use3D, float squareDistTh3D)
	{
		if (!hasCamera_ || !validCommon(cache, distThDesc, nnTh, use3D, false) ||
			!cache.hasKps1_ || !cache.hasGrid2_)
			return false;
		buildF(cache);
		return run(cache, cache.matchFImp_, makeParams(cache, distThDesc, nnTh, use3D, squareDistTh3D));
	}

	bool matchPro(ORBMatcherCacheVK& cache, GridVK& grid, int distThDesc, float nnTh,
		char use3D, float squareDistTh3D)
	{
		if (!hasCamera_ || !validCommon(cache, distThDesc, nnTh, use3D, false) ||
			!cache.hasPws1_ || !grid.uploaded_)
			return false;
		buildPro(cache, grid);
		return run(cache, cache.matchProImp_, makeParams(cache, distThDesc, nnTh, use3D, squareDistTh3D));
	}

	void downloadResult(ORBMatcherCacheVK& cache, short* matched2to1, short* matched1to2)
	{
		if (!matched2to1 || cache.kpSz1_ <= 0)
			return;
		const size_t resultSize = static_cast<size_t>(cache.kpSz1_) * sizeof(int);
		VKFence fence;
		void* data = cache.match2to1Buffer_->download(resultSize, cache.queue_, fence.ptr());
		if (!data)
			return;
		waitFence(fence);
		cache.match2to1Buffer_->invalidateBufferCopy(resultSize);
		const int* gpuMatches = static_cast<const int*>(data);
		for (int i = 0; i < cache.kpSz1_; ++i) {
			const int match = gpuMatches[i];
			if (match < 0 || match >= cache.kpSz2_)
				continue;
			matched2to1[i] = static_cast<short>(match);
			if (matched1to2 && matched1to2[match] == -1)
				matched1to2[match] = static_cast<short>(i);
		}
	}

private:
	bool validCommon(const ORBMatcherCacheVK& cache, int distThDesc, float nnTh, char use3D, bool mutual) const
	{
		if (cache.queue_ == VK_NULL_HANDLE || !cache.hasDescs1_ || !cache.hasDescs2_ ||
			cache.kpSz1_ <= 0 || cache.kpSz2_ <= 0 ||
			!std::isfinite(nnTh) || nnTh <= 0.0f || nnTh >= 1.0f || distThDesc < 0 || distThDesc > 256)
			return false;
		if (mutual && (cache.kpSz1_ < 2 || cache.kpSz2_ < 2))
			return false;
		if (use3D == 1 && (!cache.hasPcs1_ || !cache.hasPcs2_))
			return false;
		return true;
	}

	ORBMatcherParams makeParams(const ORBMatcherCacheVK& cache, int distThDesc, float nnTh,
		char use3D, float squareDistTh3D) const
	{
		ORBMatcherParams params;
		params.kpSz1 = cache.kpSz1_;
		params.kpSz2 = cache.kpSz2_;
		params.wGrid = cache.wGrid_;
		params.hGrid = cache.hGrid_;
		params.distThDesc = distThDesc;
		params.use3D = use3D == 1 ? 1 : 0;
		params.cameraModel = static_cast<int>(cameraModel_);
		params.nnTh = nnTh;
		params.squareDistTh3D = squareDistTh3D;
		params.fx = camera_[0];
		params.fy = camera_[1];
		params.cx = camera_[2];
		params.cy = camera_[3];
		params.width = static_cast<float>(width_);
		params.height = static_cast<float>(height_);
		makeTransformRows(params.Twc1Rows, cache.Twc1_);
		makeTransformRows(params.Twc2Rows, cache.Twc2_);
		makeTransformRows(params.Tcw2Rows, cache.Tcw2_);
		if (hasCamera_)
			computeFundamentalRows(params.fundamentalRows, cache.Tcw2_, cache.Twc1_, camera_.data());
		if (cameraModel_ == CameraModel::Fisheye) {
			params.distortion[0] = camera_[4];
			params.distortion[1] = camera_[5];
			params.distortion[2] = camera_[6];
			params.distortion[3] = camera_[7];
		}
		return params;
	}

	void buildBF(ORBMatcherCacheVK& cache)
	{
		if (cache.matchBFPipeline_)
			return;
		const std::string path = std::string(VULKAN_LYJ_HOME_PATH) + "/shader/orb/matchBF.comp.spv";
		cache.matchBFPipeline_.reset(new VKPipelineCompute(path));
		cache.matchBFPipeline_->setBufferBinding(0, cache.paramsBuffer_.get());
		cache.matchBFPipeline_->setBufferBinding(1, cache.descs1Buffer_.get());
		cache.matchBFPipeline_->setBufferBinding(2, cache.descs2Buffer_.get());
		cache.matchBFPipeline_->setBufferBinding(3, cache.Pcs1Buffer_.get());
		cache.matchBFPipeline_->setBufferBinding(4, cache.Pcs2Buffer_.get());
		cache.matchBFPipeline_->setBufferBinding(5, cache.match2to1Buffer_.get());
		cache.matchBFPipeline_->setRunKernel(VKORBKPSIZE, 1, 1, 256, 1, 1);
		VK_CHECK_RESULT(cache.matchBFPipeline_->build());
		cache.matchBFImp_.reset(new VKImp(0));
		cache.matchBFImp_->setCmds({ cache.matchBFPipeline_.get() });
	}

	void buildF(ORBMatcherCacheVK& cache)
	{
		if (cache.matchFPipeline_)
			return;
		const std::string path = std::string(VULKAN_LYJ_HOME_PATH) + "/shader/orb/matchF.comp.spv";
		cache.matchFPipeline_.reset(new VKPipelineCompute(path));
		cache.matchFPipeline_->setBufferBinding(0, cache.paramsBuffer_.get());
		cache.matchFPipeline_->setBufferBinding(1, cache.kps1Buffer_.get());
		cache.matchFPipeline_->setBufferBinding(2, cache.descs1Buffer_.get());
		cache.matchFPipeline_->setBufferBinding(3, cache.descs2Buffer_.get());
		cache.matchFPipeline_->setBufferBinding(4, cache.Pcs1Buffer_.get());
		cache.matchFPipeline_->setBufferBinding(5, cache.Pcs2Buffer_.get());
		cache.matchFPipeline_->setBufferBinding(6, cache.featureGrid2Buffer_.get());
		cache.matchFPipeline_->setBufferBinding(7, cache.featureGrid2SizesBuffer_.get());
		cache.matchFPipeline_->setBufferBinding(8, cache.match2to1Buffer_.get());
		cache.matchFPipeline_->setRunKernel(VKORBKPSIZE, 1, 1, 256, 1, 1);
		VK_CHECK_RESULT(cache.matchFPipeline_->build());
		cache.matchFImp_.reset(new VKImp(0));
		cache.matchFImp_->setCmds({ cache.matchFPipeline_.get() });
	}

	void buildPro(ORBMatcherCacheVK& cache, GridVK& grid)
	{
		if (cache.matchProPipeline_ && cache.boundGrid_ == &grid)
			return;
		destroyPtr(cache.matchProImp_);
		destroyPtr(cache.matchProPipeline_);
		const std::string path = std::string(VULKAN_LYJ_HOME_PATH) + "/shader/orb/matchPro.comp.spv";
		cache.matchProPipeline_.reset(new VKPipelineCompute(path));
		cache.matchProPipeline_->setBufferBinding(0, cache.paramsBuffer_.get());
		cache.matchProPipeline_->setBufferBinding(1, cache.descs1Buffer_.get());
		cache.matchProPipeline_->setBufferBinding(2, cache.descs2Buffer_.get());
		cache.matchProPipeline_->setBufferBinding(3, cache.Pcs1Buffer_.get());
		cache.matchProPipeline_->setBufferBinding(4, cache.Pcs2Buffer_.get());
		cache.matchProPipeline_->setBufferBinding(5, cache.Pws1Buffer_.get());
		cache.matchProPipeline_->setBufferBinding(6, grid.cellDatasBuffer_.get());
		cache.matchProPipeline_->setBufferBinding(7, grid.cellOffsetsBuffer_.get());
		cache.matchProPipeline_->setBufferBinding(8, cache.match2to1Buffer_.get());
		cache.matchProPipeline_->setRunKernel(VKORBKPSIZE, 1, 1, 256, 1, 1);
		VK_CHECK_RESULT(cache.matchProPipeline_->build());
		cache.matchProImp_.reset(new VKImp(0));
		cache.matchProImp_->setCmds({ cache.matchProPipeline_.get() });
		cache.boundGrid_ = &grid;
	}

	bool run(ORBMatcherCacheVK& cache, const std::shared_ptr<VKImp>& implementation,
		const ORBMatcherParams& params)
	{
		if (!implementation)
			return false;
		VKFence fence;
		cache.paramsBuffer_->upload(sizeof(params), const_cast<ORBMatcherParams*>(&params), cache.queue_, fence.ptr());
		waitFence(fence);
		implementation->run(cache.queue_, fence.ptr());
		waitFence(fence);
		return true;
	}

	int width_ = 0;
	int height_ = 0;
	bool hasCamera_ = false;
	CameraModel cameraModel_ = CameraModel::Pinhole;
	std::array<float, 8> camera_{};
};

GridVK::GridVK()
{
	init(0);
}

GridVK::GridVK(unsigned int queueIndex)
{
	init(queueIndex);
}

GridVK::~GridVK()
{
	release();
}

void GridVK::init(unsigned int queueIndex)
{
	release();
	if (!ensureVulkan())
		return;
	queueIndex_ = queueIndex;
	queue_ = getComputeQueue(queueIndex_);
	initializeComputeBuffer(cellDatasBuffer_, VKORBKPSIZE * sizeof(int));
	initializeComputeBuffer(cellOffsetsBuffer_, (gridW_ * gridH_ + 1) * sizeof(int));
}

void GridVK::upload(const short* cellDatas, const short* cellOffsets)
{
	uploaded_ = false;
	if (!cellDatasBuffer_ || !cellOffsetsBuffer_ || !cellDatas || !cellOffsets || queue_ == VK_NULL_HANDLE)
		return;
	std::vector<int> data(VKORBKPSIZE);
	std::vector<int> offsets(static_cast<size_t>(gridW_) * gridH_ + 1);
	for (size_t i = 0; i < data.size(); ++i)
		data[i] = static_cast<int>(cellDatas[i]);
	for (size_t i = 0; i < offsets.size(); ++i)
		offsets[i] = static_cast<int>(cellOffsets[i]);
	VKFence fence;
	uploadPadded(*cellDatasBuffer_, data, queue_);
	uploadPadded(*cellOffsetsBuffer_, offsets, queue_, fence.ptr());
	waitFence(fence);
	uploaded_ = true;
}

void GridVK::release()
{
	destroyPtr(cellDatasBuffer_);
	destroyPtr(cellOffsetsBuffer_);
	queue_ = VK_NULL_HANDLE;
	uploaded_ = false;
}

ORBMatcherCacheVK::ORBMatcherCacheVK()
{
	init(0);
}

ORBMatcherCacheVK::ORBMatcherCacheVK(unsigned int queueIndex)
{
	init(queueIndex);
}

ORBMatcherCacheVK::~ORBMatcherCacheVK()
{
	release();
}

void ORBMatcherCacheVK::init(unsigned int queueIndex)
{
	release();
	if (!ensureVulkan())
		return;
	queueIndex_ = queueIndex;
	queue_ = getComputeQueue(queueIndex_);
	setIdentity(Tcw1_);
	setIdentity(Twc1_);
	setIdentity(Tcw2_);
	setIdentity(Twc2_);

	paramsBuffer_.reset(new VKBufferUniform());
	paramsBuffer_->resize(sizeof(ORBMatcherParams));
	initializeComputeBuffer(kps1Buffer_, VKORBKPSIZE * 2 * sizeof(float));
	initializeComputeBuffer(descs1Buffer_, VKORBKPSIZE * 8 * sizeof(unsigned int));
	initializeComputeBuffer(descs2Buffer_, VKORBKPSIZE * 8 * sizeof(unsigned int));
	initializeComputeBuffer(Pcs1Buffer_, VKORBKPSIZE * 3 * sizeof(float));
	initializeComputeBuffer(Pcs2Buffer_, VKORBKPSIZE * 3 * sizeof(float));
	initializeComputeBuffer(Pws1Buffer_, VKORBKPSIZE * 4 * sizeof(float));
	initializeComputeBuffer(featureGrid2Buffer_, wGrid_ * hGrid_ * VKORBEVECELLSIZE * sizeof(int));
	initializeComputeBuffer(featureGrid2SizesBuffer_, wGrid_ * hGrid_ * sizeof(int));
	initializeComputeBuffer(match2to1Buffer_, VKORBKPSIZE * sizeof(int));
}

void ORBMatcherCacheVK::release()
{
	destroyPtr(matchBFImp_);
	destroyPtr(matchFImp_);
	destroyPtr(matchProImp_);
	destroyPtr(matchBFPipeline_);
	destroyPtr(matchFPipeline_);
	destroyPtr(matchProPipeline_);
	destroyPtr(paramsBuffer_);
	destroyPtr(kps1Buffer_);
	destroyPtr(descs1Buffer_);
	destroyPtr(descs2Buffer_);
	destroyPtr(Pcs1Buffer_);
	destroyPtr(Pcs2Buffer_);
	destroyPtr(Pws1Buffer_);
	destroyPtr(featureGrid2Buffer_);
	destroyPtr(featureGrid2SizesBuffer_);
	destroyPtr(match2to1Buffer_);
	boundGrid_ = nullptr;
	queue_ = VK_NULL_HANDLE;
	kpSz1_ = 0;
	kpSz2_ = 0;
	hasKps1_ = false;
	hasDescs1_ = false;
	hasDescs2_ = false;
	hasPcs1_ = false;
	hasPcs2_ = false;
	hasPws1_ = false;
	hasGrid2_ = false;
}

void ORBMatcherCacheVK::upload1(int kpSize, const float* Tcw, const float* Twc,
	const float* keypoints, const unsigned int* descriptors,
	const float* Pcs, const float* Pws, const char* validPws)
{
	hasDescs1_ = descriptors != nullptr;
	kpSz1_ = hasDescs1_ ? std::clamp(kpSize, 0, VKORBKPSIZE) : 0;
	copyTransform(Tcw1_, Tcw);
	copyTransform(Twc1_, Twc);
	hasKps1_ = keypoints != nullptr;
	hasPcs1_ = Pcs != nullptr;
	hasPws1_ = Pws != nullptr && validPws != nullptr;
	if (kpSz1_ == 0 || !descriptors || queue_ == VK_NULL_HANDLE)
		return;
	if (keypoints)
		uploadPadded(*kps1Buffer_, keypoints, static_cast<size_t>(kpSz1_) * 2, queue_);
	if (Pcs)
		uploadPadded(*Pcs1Buffer_, Pcs, static_cast<size_t>(kpSz1_) * 3, queue_);
	if (hasPws1_) {
		std::vector<float> packed(static_cast<size_t>(kpSz1_) * 4);
		for (int i = 0; i < kpSz1_; ++i) {
			packed[4 * i] = Pws[3 * i];
			packed[4 * i + 1] = Pws[3 * i + 1];
			packed[4 * i + 2] = Pws[3 * i + 2];
			packed[4 * i + 3] = validPws[i] == 0 ? 0.0f : 1.0f;
		}
		uploadPadded(*Pws1Buffer_, packed, queue_);
	}
	VKFence fence;
	uploadPadded(*descs1Buffer_, descriptors, static_cast<size_t>(kpSz1_) * 8, queue_, fence.ptr());
	waitFence(fence);
}

void ORBMatcherCacheVK::upload2(int kpSize, const float* Tcw, const float* Twc,
	const short* featureGrid, const char* featureGridSizes,
	const float* keypoints, const unsigned int* descriptors, const float* Pcs)
{
	hasDescs2_ = descriptors != nullptr;
	kpSz2_ = hasDescs2_ ? std::clamp(kpSize, 0, VKORBKPSIZE) : 0;
	copyTransform(Tcw2_, Tcw);
	copyTransform(Twc2_, Twc);
	hasPcs2_ = Pcs != nullptr;
	hasGrid2_ = featureGrid != nullptr && featureGridSizes != nullptr;
	if (kpSz2_ == 0 || !descriptors || queue_ == VK_NULL_HANDLE)
		return;
	if (hasGrid2_) {
		const size_t gridCount = static_cast<size_t>(wGrid_) * hGrid_ * VKORBEVECELLSIZE;
		const size_t cellCount = static_cast<size_t>(wGrid_) * hGrid_;
		std::vector<int> grid(gridCount);
		std::vector<int> sizes(cellCount);
		for (size_t i = 0; i < gridCount; ++i)
			grid[i] = static_cast<int>(featureGrid[i]);
		for (size_t i = 0; i < cellCount; ++i)
			sizes[i] = std::min<int>(static_cast<unsigned char>(featureGridSizes[i]), VKORBEVECELLSIZE);
		uploadPadded(*featureGrid2Buffer_, grid, queue_);
		uploadPadded(*featureGrid2SizesBuffer_, sizes, queue_);
	}
	if (Pcs)
		uploadPadded(*Pcs2Buffer_, Pcs, static_cast<size_t>(kpSz2_) * 3, queue_);
	VKFence fence;
	uploadPadded(*descs2Buffer_, descriptors, static_cast<size_t>(kpSz2_) * 8, queue_, fence.ptr());
	waitFence(fence);
}

void ORBMatcherCacheVK::upload1(int kpSize, const unsigned int* descriptors)
{
	hasDescs1_ = descriptors != nullptr;
	hasKps1_ = false;
	hasPcs1_ = false;
	hasPws1_ = false;
	kpSz1_ = hasDescs1_ ? std::clamp(kpSize, 0, VKORBKPSIZE) : 0;
	if (kpSz1_ == 0 || queue_ == VK_NULL_HANDLE)
		return;
	VKFence fence;
	uploadPadded(*descs1Buffer_, descriptors, static_cast<size_t>(kpSz1_) * 8, queue_, fence.ptr());
	waitFence(fence);
}

void ORBMatcherCacheVK::upload2(int kpSize, const unsigned int* descriptors)
{
	hasDescs2_ = descriptors != nullptr;
	hasPcs2_ = false;
	hasGrid2_ = false;
	kpSz2_ = hasDescs2_ ? std::clamp(kpSize, 0, VKORBKPSIZE) : 0;
	if (kpSz2_ == 0 || queue_ == VK_NULL_HANDLE)
		return;
	VKFence fence;
	uploadPadded(*descs2Buffer_, descriptors, static_cast<size_t>(kpSz2_) * 8, queue_, fence.ptr());
	waitFence(fence);
}

namespace {

void clearMatchResult(ORBMatcherCacheVK& cache, short* matched2to1, short* matched1to2)
{
	if (matched2to1)
		std::fill(matched2to1, matched2to1 + cache.kpSz1_, static_cast<short>(-1));
	if (matched1to2)
		std::fill(matched1to2, matched1to2 + cache.kpSz2_, static_cast<short>(-1));
}

template <typename MatchCall>
void runMatcher(void* handle, ORBMatcherCacheVK& cache,
	short* matched2to1, short* matched1to2, MatchCall&& call)
{
	clearMatchResult(cache, matched2to1, matched1to2);
	if (!handle || !matched2to1)
		return;
	auto* matcher = static_cast<ORBMatcherVK*>(handle);
	if (!call(*matcher))
		return;
	matcher->downloadResult(cache, matched2to1, matched1to2);
}

} // namespace

VULKAN_LYJ_API void* initMatcherVK(int width, int height, const float* camera, CameraModel cameraModel)
{
	try {
		return new ORBMatcherVK(width, height, camera, cameraModel);
	}
	catch (const std::exception&) {
		return nullptr;
	}
}

VULKAN_LYJ_API void matchBFVK(void* handle, ORBMatcherCacheVK& cache,
	short* matched2to1, short* matched1to2,
	int distThDesc, float nnTh, char checkOrientation, char use3D, float squareDistTh3D)
{
	(void)checkOrientation;
	runMatcher(handle, cache, matched2to1, matched1to2,
		[&](ORBMatcherVK& matcher) { return matcher.matchBF(cache, distThDesc, nnTh, use3D, squareDistTh3D); });
}

VULKAN_LYJ_API void matchFVK(void* handle, ORBMatcherCacheVK& cache,
	short* matched2to1, short* matched1to2,
	int distThDesc, float nnTh, char checkOrientation, char use3D, float squareDistTh3D)
{
	(void)checkOrientation;
	runMatcher(handle, cache, matched2to1, matched1to2,
		[&](ORBMatcherVK& matcher) { return matcher.matchF(cache, distThDesc, nnTh, use3D, squareDistTh3D); });
}

VULKAN_LYJ_API void matchProVK(void* handle, ORBMatcherCacheVK& cache, GridVK& grid,
	short* matched2to1, short* matched1to2,
	int distThDesc, float nnTh, char checkOrientation, char use3D, float squareDistTh3D)
{
	(void)checkOrientation;
	runMatcher(handle, cache, matched2to1, matched1to2,
		[&](ORBMatcherVK& matcher) { return matcher.matchPro(cache, grid, distThDesc, nnTh, use3D, squareDistTh3D); });
}

VULKAN_LYJ_API void releaseMatcherVK(void* handle)
{
	delete static_cast<ORBMatcherVK*>(handle);
}

NSP_VULKAN_LYJ_END
