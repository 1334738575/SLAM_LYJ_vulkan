#ifndef VULKAN_INCLUDE_H
#define VULKAN_INCLUDE_H


#include "VulkanDefines.h"
#include "VulkanORBMatcher.h"
#include <stdint.h>
#include <vector>


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
	float minD, float maxD, float csTh = 0, float detDTh = 1,
	std::vector<uint32_t>* faceIds = nullptr,
	std::vector<uint32_t>* pointIds = nullptr);
VULKAN_LYJ_API void releaseVK(ProVKHandle handle);

typedef void* MatchVKHandle;
VULKAN_LYJ_API MatchVKHandle initMatcherVK(int width = 0, int height = 0, const float* camera = nullptr);
VULKAN_LYJ_API void matchBFVK(MatchVKHandle handle, ORBMatcherCacheVK& cache,
	short* matched2to1, short* matched1to2,
	int distThDesc = 64, float nnTh = 0.8f, char checkOrientation = 0,
	char use3D = 0, float squareDistTh3D = 0.0f);
VULKAN_LYJ_API void matchFVK(MatchVKHandle handle, ORBMatcherCacheVK& cache,
	short* matched2to1, short* matched1to2,
	int distThDesc, float nnTh, char checkOrientation, char use3D, float squareDistTh3D);
VULKAN_LYJ_API void matchProVK(MatchVKHandle handle, ORBMatcherCacheVK& cache, GridVK& grid,
	short* matched2to1, short* matched1to2,
	int distThDesc, float nnTh, char checkOrientation, char use3D, float squareDistTh3D);
VULKAN_LYJ_API void releaseMatcherVK(MatchVKHandle handle);



NSP_VULKAN_LYJ_END


#endif // !VULKAN_INCLUDE_H
