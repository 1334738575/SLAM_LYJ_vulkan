#ifndef VULKAN_INCLUDE_H
#define VULKAN_INCLUDE_H


#include "VulkanDefines.h"


NSP_VULKAN_LYJ_BEGIN

typedef void* ProVKHandle;
typedef void* ProVKCacheHandle;
VULKAN_LYJ_API ProVKHandle initProjectorVK(
	const float* Pws, const unsigned int PSize,
	const float* centers, const float* fNormals, const unsigned int* faces, const unsigned int fSize,
	float* camParams, const int w, const int h);
VULKAN_LYJ_API unsigned int getProjectorVKQueueCount(ProVKHandle handle);
VULKAN_LYJ_API ProVKCacheHandle initProjectorVKCache(ProVKHandle handle, unsigned int queueIndex = 0);
VULKAN_LYJ_API void releaseProjectorVKCache(ProVKCacheHandle cacheHandle);
VULKAN_LYJ_API void projectVK(ProVKHandle handle, ProVKCacheHandle cacheHandle,
	float* Tcw,
	float* depths, unsigned int* fIds, char* allVisiblePIds, char* allVisibleFIds,
	float minD, float maxD, float csTh = 0, float detDTh = 1);
VULKAN_LYJ_API void releaseVK(ProVKHandle handle);



NSP_VULKAN_LYJ_END


#endif // !VULKAN_INCLUDE_H
