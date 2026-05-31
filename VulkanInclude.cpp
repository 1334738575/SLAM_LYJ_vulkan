#include "VulkanInclude.h"
#include "projector/ProjectorVK.h"


NSP_VULKAN_LYJ_BEGIN


VULKAN_LYJ_API ProVKHandle initProjectorVK(const float* Pws, const unsigned int PSize, const float* centers, const float* fNormals, const unsigned int* faces, const unsigned int fSize, float* camParams, const int w, const int h)
{
	ProjectorVK* pro = new ProjectorVK();
	if (!pro->create(Pws, PSize, centers, fNormals, faces, fSize, camParams, w, h))
	{
		delete pro;
		return nullptr;
	}
	return pro;
}

VULKAN_LYJ_API unsigned int getProjectorVKQueueCount(ProVKHandle handle)
{
	if (!handle)
		return 0;
	ProjectorVK* pro = (ProjectorVK*)handle;
	return pro->getQueueCount();
}

VULKAN_LYJ_API ProVKCacheHandle initProjectorVKCache(ProVKHandle handle, unsigned int queueIndex)
{
	if (!handle)
		return nullptr;
	ProjectorVK* pro = (ProjectorVK*)handle;
	if (queueIndex >= pro->getQueueCount())
		return nullptr;

	ProjectorCacheVK* cache = new ProjectorCacheVK();
	cache->init(pro->PSize, pro->fSize, pro->w_, pro->h_, queueIndex);
	return cache;
}

VULKAN_LYJ_API void releaseProjectorVKCache(ProVKCacheHandle cacheHandle)
{
	if (!cacheHandle)
		return;
	ProjectorCacheVK* cache = (ProjectorCacheVK*)cacheHandle;
	cache->release();
	delete cache;
}

VULKAN_LYJ_API void projectVK(ProVKHandle handle, ProVKCacheHandle cacheHandle, float* Tcw, float* depths, unsigned int* fIds, char* allVisiblePIds, char* allVisibleFIds, float minD, float maxD, float csTh, float detDTh)
{
	if (!handle || !cacheHandle)
		return;
	ProjectorVK* pro = (ProjectorVK*)handle;
	ProjectorCacheVK* cache = (ProjectorCacheVK*)cacheHandle;
	pro->project(*cache, Tcw, depths, fIds, allVisiblePIds, allVisibleFIds, minD, maxD, csTh, detDTh);
}

VULKAN_LYJ_API void releaseVK(ProVKHandle handle)
{
	if (!handle)
		return;
	ProjectorVK* pro = (ProjectorVK*)handle;
	pro->release();
	delete pro;
	return;
}


NSP_VULKAN_LYJ_END
