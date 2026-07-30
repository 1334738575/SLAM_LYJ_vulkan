#ifndef VULKAN_LYJ_SIFT_MATCHER_H
#define VULKAN_LYJ_SIFT_MATCHER_H

#include "VulkanDefines.h"

#include <array>
#include <memory>

#define VKSIFTKPSIZE 8192
#define VKSIFTDESCSIZE 128

NSP_VULKAN_LYJ_BEGIN

class SIFTMatcherVK;

class VULKAN_LYJ_API SIFTMatcherCacheVK
{
public:
	SIFTMatcherCacheVK();
	explicit SIFTMatcherCacheVK(unsigned int queueIndex);
	~SIFTMatcherCacheVK();

	void init(unsigned int queueIndex = 0);
	void release();

	// SiftGPU angular distance requires L2-normalized descriptors.
	void upload1(int kpSize, const float* Twc, const float* descriptors, const float* Pcs = nullptr,
		bool normalizeDescriptors = true);
	void upload2(int kpSize, const float* Twc, const float* descriptors, const float* Pcs = nullptr,
		bool normalizeDescriptors = true);
	void upload1(int kpSize, const float* descriptors, bool normalizeDescriptors = true);
	void upload2(int kpSize, const float* descriptors, bool normalizeDescriptors = true);

	int kpSz1_ = 0;
	int kpSz2_ = 0;

private:
	friend class SIFTMatcherVK;

	unsigned int queueIndex_ = 0;
	VkQueue queue_ = VK_NULL_HANDLE;
	bool hasDescs1_ = false;
	bool hasDescs2_ = false;
	bool hasPcs1_ = false;
	bool hasPcs2_ = false;
	std::array<float, 12> Twc1_{};
	std::array<float, 12> Twc2_{};

	std::shared_ptr<VKBufferUniform> paramsBuffer_;
	std::shared_ptr<VKBufferCompute> descs1Buffer_;
	std::shared_ptr<VKBufferCompute> descs2Buffer_;
	std::shared_ptr<VKBufferCompute> Pcs1Buffer_;
	std::shared_ptr<VKBufferCompute> Pcs2Buffer_;
	std::shared_ptr<VKBufferCompute> match2to1Buffer_;
	std::shared_ptr<VKPipelineCompute> matchBFPipeline_;
	std::shared_ptr<VKImp> matchBFImp_;
};

NSP_VULKAN_LYJ_END

#endif // VULKAN_LYJ_SIFT_MATCHER_H
