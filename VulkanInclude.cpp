#include "VulkanInclude.h"
#include "projector/ProjectorVK.h"


NSP_VULKAN_LYJ_BEGIN

namespace {

	struct ProjectorVKHandle
	{
		ProjectorVK projector;
		ProjectorCacheVK cache;
	};

} // namespace


VULKAN_LYJ_API ProVKHandle initProjectorVK(const float* Pws, const unsigned int PSize, const float* centers, const float* fNormals, const unsigned int* faces, const unsigned int fSize, float* camParams, const int w, const int h)
{
	ProjectorVKHandle* pro = new ProjectorVKHandle();
	if (!pro->projector.create(Pws, PSize, centers, fNormals, faces, fSize, camParams, w, h))
	{
		delete pro;
		return nullptr;
	}
	pro->cache.init(PSize, fSize, w, h);
	return (void*)pro;
}

VULKAN_LYJ_API void projectVK(ProVKHandle handle, float* Tcw, float* depths, unsigned int* fIds, char* allVisiblePIds, char* allVisibleFIds, float minD, float maxD, float csTh, float detDTh)
{
	if (!handle)
		return;
	ProjectorVKHandle* pro = (ProjectorVKHandle*)handle;
	pro->projector.project(pro->cache, Tcw, depths, fIds, allVisiblePIds, allVisibleFIds, minD, maxD, csTh, detDTh);
}

VULKAN_LYJ_API void releaseVK(ProVKHandle handle)
{
	if (!handle)
		return;
	ProjectorVKHandle* pro = (ProjectorVKHandle*)handle;
	pro->cache.release();
	pro->projector.release();
	delete pro;
	return;
}


NSP_VULKAN_LYJ_END
