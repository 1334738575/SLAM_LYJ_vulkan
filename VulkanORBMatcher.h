#ifndef VULKAN_LYJ_ORB_MATCHER_H
#define VULKAN_LYJ_ORB_MATCHER_H

#include "VulkanDefines.h"

#include <array>
#include <memory>

#define VKORBMAXW 2000
#define VKORBMAXH 2000
#define VKORBKPSIZE 8192
#define VKORBGRIDSOLU 20
#define VKORBEVECELLSIZE 128

NSP_VULKAN_LYJ_BEGIN

class ORBMatcherVK;

class VULKAN_LYJ_API GridVK
{
public:
	GridVK();
	explicit GridVK(unsigned int queueIndex);
	~GridVK();

	void upload(const short* cellDatas, const short* cellOffsets);
	void release();

	int gridW_ = (VKORBMAXW + VKORBGRIDSOLU - 1) / VKORBGRIDSOLU;
	int gridH_ = (VKORBMAXH + VKORBGRIDSOLU - 1) / VKORBGRIDSOLU;

private:
	friend class ORBMatcherVK;
	void init(unsigned int queueIndex);

	unsigned int queueIndex_ = 0;
	VkQueue queue_ = VK_NULL_HANDLE;
	bool uploaded_ = false;
	std::shared_ptr<VKBufferCompute> cellDatasBuffer_;
	std::shared_ptr<VKBufferCompute> cellOffsetsBuffer_;
};

class VULKAN_LYJ_API ORBMatcherCacheVK
{
public:
	ORBMatcherCacheVK();
	explicit ORBMatcherCacheVK(unsigned int queueIndex);
	~ORBMatcherCacheVK();

	void init(unsigned int queueIndex = 0);
	void release();

	// The 3D inputs are optional when descriptor-only matching is requested.
	void upload1(int kpSize, const float* Tcw, const float* Twc,
		const float* keypoints, const unsigned int* descriptors,
		const float* Pcs = nullptr, const float* Pws = nullptr, const char* validPws = nullptr);
	void upload2(int kpSize, const float* Tcw, const float* Twc,
		const short* featureGrid, const char* featureGridSizes,
		const float* keypoints, const unsigned int* descriptors,
		const float* Pcs = nullptr);
	void upload1(int kpSize, const unsigned int* descriptors);
	void upload2(int kpSize, const unsigned int* descriptors);

	int wGrid_ = (VKORBMAXW + VKORBGRIDSOLU - 1) / VKORBGRIDSOLU;
	int hGrid_ = (VKORBMAXH + VKORBGRIDSOLU - 1) / VKORBGRIDSOLU;
	int kpSz1_ = 0;
	int kpSz2_ = 0;

private:
	friend class ORBMatcherVK;

	unsigned int queueIndex_ = 0;
	VkQueue queue_ = VK_NULL_HANDLE;
	bool hasKps1_ = false;
	bool hasDescs1_ = false;
	bool hasDescs2_ = false;
	bool hasPcs1_ = false;
	bool hasPcs2_ = false;
	bool hasPws1_ = false;
	bool hasGrid2_ = false;
	std::array<float, 12> Tcw1_{};
	std::array<float, 12> Twc1_{};
	std::array<float, 12> Tcw2_{};
	std::array<float, 12> Twc2_{};

	std::shared_ptr<VKBufferUniform> paramsBuffer_;
	std::shared_ptr<VKBufferCompute> kps1Buffer_;
	std::shared_ptr<VKBufferCompute> descs1Buffer_;
	std::shared_ptr<VKBufferCompute> descs2Buffer_;
	std::shared_ptr<VKBufferCompute> Pcs1Buffer_;
	std::shared_ptr<VKBufferCompute> Pcs2Buffer_;
	std::shared_ptr<VKBufferCompute> Pws1Buffer_;
	std::shared_ptr<VKBufferCompute> featureGrid2Buffer_;
	std::shared_ptr<VKBufferCompute> featureGrid2SizesBuffer_;
	std::shared_ptr<VKBufferCompute> match2to1Buffer_;

	std::shared_ptr<VKPipelineCompute> matchBFPipeline_;
	std::shared_ptr<VKPipelineCompute> matchFPipeline_;
	std::shared_ptr<VKPipelineCompute> matchProPipeline_;
	std::shared_ptr<VKImp> matchBFImp_;
	std::shared_ptr<VKImp> matchFImp_;
	std::shared_ptr<VKImp> matchProImp_;
	GridVK* boundGrid_ = nullptr;
};

NSP_VULKAN_LYJ_END

#endif // VULKAN_LYJ_ORB_MATCHER_H
