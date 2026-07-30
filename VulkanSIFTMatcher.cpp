#include "VulkanSIFTMatcher.h"

#include "VulkanConfig.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

NSP_VULKAN_LYJ_BEGIN

namespace {

struct alignas(16) SIFTMatcherParams
{
	int kpSz1 = 0;
	int kpSz2 = 0;
	int mutualBestMatch = 0;
	int use3D = 0;

	float distMax = 0.0f;
	float ratioMax = 0.0f;
	float squareDistTh3D = 0.0f;
	float padding0 = 0.0f;

	float Twc1Rows[12]{};
	float Twc2Rows[12]{};
};

static_assert(sizeof(SIFTMatcherParams) == 128, "SIFT matcher UBO layout must match GLSL std140 layout");

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
void uploadPadded(VKBufferAbr& buffer, const T* data, size_t count, VkQueue queue,
	VkFence fence = VK_NULL_HANDLE)
{
	if (count == 0)
		return;
	const size_t byteSize = count * sizeof(T);
	const size_t paddedSize = (byteSize + 63) / 64 * 64;
	std::vector<unsigned char> upload(paddedSize, 0);
	std::memcpy(upload.data(), data, byteSize);
	buffer.upload(static_cast<VkDeviceSize>(paddedSize), upload.data(), queue, fence);
}

void initializeComputeBuffer(std::shared_ptr<VKBufferCompute>& buffer, VkDeviceSize size)
{
	buffer.reset(new VKBufferCompute());
	buffer->resize(size);
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

void waitFence(VKFence& fence)
{
	fence.wait();
	fence.reset();
}

std::vector<float> prepareDescriptors(int descriptorCount, const float* descriptors,
	bool normalizeDescriptors)
{
	const size_t valueCount = static_cast<size_t>(descriptorCount) * VKSIFTDESCSIZE;
	std::vector<float> prepared(valueCount);
	if (!normalizeDescriptors) {
		std::copy(descriptors, descriptors + valueCount, prepared.begin());
		return prepared;
	}
	for (int descriptorIndex = 0; descriptorIndex < descriptorCount; ++descriptorIndex) {
		const float* source = descriptors + static_cast<size_t>(descriptorIndex) * VKSIFTDESCSIZE;
		float* destination = prepared.data() + static_cast<size_t>(descriptorIndex) * VKSIFTDESCSIZE;
		float squareNorm = 0.0f;
		for (int i = 0; i < VKSIFTDESCSIZE; ++i)
			squareNorm += source[i] * source[i];
		const float scale = squareNorm > 0.0f ? 1.0f / std::sqrt(squareNorm) : 0.0f;
		for (int i = 0; i < VKSIFTDESCSIZE; ++i)
			destination[i] = source[i] * scale;
	}
	return prepared;
}

} // namespace

class SIFTMatcherVK
{
public:
	SIFTMatcherVK()
	{
		if (!ensureVulkan())
			throw std::runtime_error("failed to initialize Vulkan for SIFT matcher");
	}

	bool matchBF(SIFTMatcherCacheVK& cache, float distMax, float ratioMax,
		char mutualBestMatch, char use3D, float squareDistTh3D)
	{
		if (!valid(cache, distMax, ratioMax, use3D, squareDistTh3D))
			return false;
		build(cache);
		SIFTMatcherParams params;
		params.kpSz1 = cache.kpSz1_;
		params.kpSz2 = cache.kpSz2_;
		params.mutualBestMatch = mutualBestMatch != 0 ? 1 : 0;
		params.use3D = use3D == 1 ? 1 : 0;
		params.distMax = distMax;
		params.ratioMax = ratioMax;
		params.squareDistTh3D = squareDistTh3D;
		makeTransformRows(params.Twc1Rows, cache.Twc1_);
		makeTransformRows(params.Twc2Rows, cache.Twc2_);

		VKFence fence;
		cache.paramsBuffer_->upload(sizeof(params), &params, cache.queue_, fence.ptr());
		waitFence(fence);
		cache.matchBFImp_->run(cache.queue_, fence.ptr());
		waitFence(fence);
		return true;
	}

	void downloadResult(SIFTMatcherCacheVK& cache, short* matched2to1, short* matched1to2)
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
	bool valid(const SIFTMatcherCacheVK& cache, float distMax, float ratioMax,
		char use3D, float squareDistTh3D) const
	{
		if (cache.queue_ == VK_NULL_HANDLE || !cache.hasDescs1_ || !cache.hasDescs2_ ||
			cache.kpSz1_ <= 0 || cache.kpSz2_ <= 0 ||
			!std::isfinite(distMax) || distMax <= 0.0f ||
			!std::isfinite(ratioMax) || ratioMax <= 0.0f)
			return false;
		if (use3D == 1 && (!cache.hasPcs1_ || !cache.hasPcs2_ ||
			!std::isfinite(squareDistTh3D) || squareDistTh3D < 0.0f))
			return false;
		return true;
	}

	void build(SIFTMatcherCacheVK& cache)
	{
		if (cache.matchBFPipeline_)
			return;
		const std::string path = std::string(VULKAN_LYJ_HOME_PATH) + "/shader/sift/matchBF.comp.spv";
		cache.matchBFPipeline_.reset(new VKPipelineCompute(path));
		cache.matchBFPipeline_->setBufferBinding(0, cache.paramsBuffer_.get());
		cache.matchBFPipeline_->setBufferBinding(1, cache.descs1Buffer_.get());
		cache.matchBFPipeline_->setBufferBinding(2, cache.descs2Buffer_.get());
		cache.matchBFPipeline_->setBufferBinding(3, cache.Pcs1Buffer_.get());
		cache.matchBFPipeline_->setBufferBinding(4, cache.Pcs2Buffer_.get());
		cache.matchBFPipeline_->setBufferBinding(5, cache.match2to1Buffer_.get());
		cache.matchBFPipeline_->setRunKernel(VKSIFTKPSIZE, 1, 1, 256, 1, 1);
		VK_CHECK_RESULT(cache.matchBFPipeline_->build());
		cache.matchBFImp_.reset(new VKImp(0));
		cache.matchBFImp_->setCmds({ cache.matchBFPipeline_.get() });
	}
};

SIFTMatcherCacheVK::SIFTMatcherCacheVK()
{
	init(0);
}

SIFTMatcherCacheVK::SIFTMatcherCacheVK(unsigned int queueIndex)
{
	init(queueIndex);
}

SIFTMatcherCacheVK::~SIFTMatcherCacheVK()
{
	release();
}

void SIFTMatcherCacheVK::init(unsigned int queueIndex)
{
	release();
	if (!ensureVulkan())
		return;
	queueIndex_ = queueIndex;
	queue_ = getComputeQueue(queueIndex_);
	setIdentity(Twc1_);
	setIdentity(Twc2_);
	paramsBuffer_.reset(new VKBufferUniform());
	paramsBuffer_->resize(sizeof(SIFTMatcherParams));
	initializeComputeBuffer(descs1Buffer_, VKSIFTKPSIZE * VKSIFTDESCSIZE * sizeof(float));
	initializeComputeBuffer(descs2Buffer_, VKSIFTKPSIZE * VKSIFTDESCSIZE * sizeof(float));
	initializeComputeBuffer(Pcs1Buffer_, VKSIFTKPSIZE * 3 * sizeof(float));
	initializeComputeBuffer(Pcs2Buffer_, VKSIFTKPSIZE * 3 * sizeof(float));
	initializeComputeBuffer(match2to1Buffer_, VKSIFTKPSIZE * sizeof(int));
}

void SIFTMatcherCacheVK::release()
{
	destroyPtr(matchBFImp_);
	destroyPtr(matchBFPipeline_);
	destroyPtr(paramsBuffer_);
	destroyPtr(descs1Buffer_);
	destroyPtr(descs2Buffer_);
	destroyPtr(Pcs1Buffer_);
	destroyPtr(Pcs2Buffer_);
	destroyPtr(match2to1Buffer_);
	queue_ = VK_NULL_HANDLE;
	kpSz1_ = 0;
	kpSz2_ = 0;
	hasDescs1_ = false;
	hasDescs2_ = false;
	hasPcs1_ = false;
	hasPcs2_ = false;
}

void SIFTMatcherCacheVK::upload1(int kpSize, const float* Twc, const float* descriptors,
	const float* Pcs, bool normalizeDescriptors)
{
	hasDescs1_ = descriptors != nullptr;
	hasPcs1_ = Pcs != nullptr;
	kpSz1_ = hasDescs1_ ? std::clamp(kpSize, 0, VKSIFTKPSIZE) : 0;
	copyTransform(Twc1_, Twc);
	if (kpSz1_ == 0 || queue_ == VK_NULL_HANDLE)
		return;
	if (Pcs)
		uploadPadded(*Pcs1Buffer_, Pcs, static_cast<size_t>(kpSz1_) * 3, queue_);
	std::vector<float> prepared = prepareDescriptors(kpSz1_, descriptors, normalizeDescriptors);
	VKFence fence;
	uploadPadded(*descs1Buffer_, prepared.data(), prepared.size(), queue_, fence.ptr());
	waitFence(fence);
}

void SIFTMatcherCacheVK::upload2(int kpSize, const float* Twc, const float* descriptors,
	const float* Pcs, bool normalizeDescriptors)
{
	hasDescs2_ = descriptors != nullptr;
	hasPcs2_ = Pcs != nullptr;
	kpSz2_ = hasDescs2_ ? std::clamp(kpSize, 0, VKSIFTKPSIZE) : 0;
	copyTransform(Twc2_, Twc);
	if (kpSz2_ == 0 || queue_ == VK_NULL_HANDLE)
		return;
	if (Pcs)
		uploadPadded(*Pcs2Buffer_, Pcs, static_cast<size_t>(kpSz2_) * 3, queue_);
	std::vector<float> prepared = prepareDescriptors(kpSz2_, descriptors, normalizeDescriptors);
	VKFence fence;
	uploadPadded(*descs2Buffer_, prepared.data(), prepared.size(), queue_, fence.ptr());
	waitFence(fence);
}

void SIFTMatcherCacheVK::upload1(int kpSize, const float* descriptors, bool normalizeDescriptors)
{
	upload1(kpSize, nullptr, descriptors, nullptr, normalizeDescriptors);
}

void SIFTMatcherCacheVK::upload2(int kpSize, const float* descriptors, bool normalizeDescriptors)
{
	upload2(kpSize, nullptr, descriptors, nullptr, normalizeDescriptors);
}

namespace {

void clearMatchResult(SIFTMatcherCacheVK& cache, short* matched2to1, short* matched1to2)
{
	if (matched2to1)
		std::fill(matched2to1, matched2to1 + cache.kpSz1_, static_cast<short>(-1));
	if (matched1to2)
		std::fill(matched1to2, matched1to2 + cache.kpSz2_, static_cast<short>(-1));
}

} // namespace

VULKAN_LYJ_API void* initSIFTMatcherVK()
{
	try {
		return new SIFTMatcherVK();
	}
	catch (const std::exception&) {
		return nullptr;
	}
}

VULKAN_LYJ_API void matchBFVK(void* handle, SIFTMatcherCacheVK& cache,
	short* matched2to1, short* matched1to2,
	float distMax, float ratioMax, char mutualBestMatch, char use3D, float squareDistTh3D)
{
	clearMatchResult(cache, matched2to1, matched1to2);
	if (!handle || !matched2to1)
		return;
	auto* matcher = static_cast<SIFTMatcherVK*>(handle);
	if (!matcher->matchBF(cache, distMax, ratioMax, mutualBestMatch, use3D, squareDistTh3D))
		return;
	matcher->downloadResult(cache, matched2to1, matched1to2);
}

VULKAN_LYJ_API void releaseSIFTMatcherVK(void* handle)
{
	delete static_cast<SIFTMatcherVK*>(handle);
}

NSP_VULKAN_LYJ_END
