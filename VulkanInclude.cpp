#include "VulkanInclude.h"
#include "projector/ProjectorVK.h"


NSP_VULKAN_LYJ_BEGIN


VULKAN_LYJ_API ProVKHandle initProjectorVK(const float* Pws, const unsigned int PSize, const float* centers, const float* fNormals, const unsigned int* faces, const unsigned int fSize, float* camParams, const int w, const int h)
{
	ProjectorVK* pro = new ProjectorVK();
	pro->create(Pws, PSize, centers, fNormals, faces, fSize, camParams, w, h);
	return (void*)pro;
}

VULKAN_LYJ_API void projectVK(ProVKHandle handle, float* Tcw, float* depths, unsigned int* fIds, char* allVisiblePIds, char* allVisibleFIds, float minD, float maxD, float csTh, float detDTh)
{
	ProjectorVK* pro = (ProjectorVK*)handle;
	pro->project(Tcw, depths, fIds, allVisiblePIds, allVisibleFIds, minD, maxD, csTh, detDTh);
}

VULKAN_LYJ_API void releaseVK(ProVKHandle handle)
{
	ProjectorVK* pro = (ProjectorVK*)handle;
	pro->release();
	delete pro;
	return;
}


NSP_VULKAN_LYJ_END
