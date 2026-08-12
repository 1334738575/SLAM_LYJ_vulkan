#ifndef VULKAN_DEFINES_H
#define VULKAN_DEFINES_H


#include "VulkanCommon.h"
#include "VulkanBuffer.h"
#include "VulkanPipeline.h"
#include "VulkanSwapChain.h"
#include "VulkanImplement.h"


NSP_VULKAN_LYJ_BEGIN

enum class CameraModel : uint32_t
{
	Pinhole = 0,
	Fisheye = 1,
};




NSP_VULKAN_LYJ_END


#endif // !VULKAN_DEFINES_H
