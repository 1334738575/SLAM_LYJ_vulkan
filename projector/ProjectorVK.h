#ifndef VULKAN_PROJECTOR_H
#define VULKAN_PROJECTOR_H

#include "VulkanDefines.h"


NSP_VULKAN_LYJ_BEGIN

struct UBOProjectCompute
{
    uint32_t vSize;
    uint32_t fSize;
    float fx;
    float fy;
    float cx;
    float cy;
    uint32_t vStep;
    uint32_t fStep;
    uint32_t w;
    uint32_t h;
    float detd;
    float maxD;
    float minD;
    float csTh;
    uint32_t dStep;
};

struct UBOProjectGraph
{
    float halfW;
    float halfH;
    float maxD;
};


class VULKAN_LYJ_API ProjectorVK
{
public:
    ProjectorVK();
    ~ProjectorVK();

    bool create(const float* Pws, const unsigned int PSize,
        const float* centers, const float* fNormals, const unsigned int* faces, const unsigned int fSize,
        float* camParams, const int w, const int h);
    void project(float* Tcw,
        float* depths, unsigned int* fIds, char* allVisiblePIds, char* allVisibleFIds,
        float minD, float maxD, float csTh, float detDTh);
    void release();

private:
    LYJ_VK::VKInstance* lyjVK = nullptr;

    UBOProjectCompute uboComCPU_;
    UBOProjectGraph uboGraphCPU_;
    std::vector<uint32_t> fIds_;
    std::vector<float> depths_;
    std::vector<uint32_t> PValids_;
    std::vector<uint32_t> fValids_;

    uint32_t kernel_ = 1024;
    VkDeviceSize PBufferSize;
    VkDeviceSize fBufferSize;
    std::shared_ptr<LYJ_VK::VKFence> fence_;
    VkQueue queue;
    VkQueue graphicQueue;
    VkDeviceSize fIdsBufferSize;
    VkDeviceSize depthsBufferSize;
    VkDeviceSize PValidsBufferSize;
    VkDeviceSize fValidsBufferSize;

    std::vector<std::shared_ptr<LYJ_VK::VKCommandAbr>> cmdBars;

    //in
    std::shared_ptr<LYJ_VK::VKBufferCompute> PwsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fcwsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fnsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> TBuffer;

    //cache
    std::shared_ptr<LYJ_VK::VKBufferUniform> uboCom;
    std::shared_ptr<LYJ_VK::VKBufferUniform> uboGraph;
    std::shared_ptr<LYJ_VK::VKBufferCompute> PcsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> uvPsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fccsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fncsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> uvfcsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferVertex> verBuffer;
    std::shared_ptr<LYJ_VK::VKBufferIndex> indBuffer;

    //out
    std::shared_ptr<LYJ_VK::VKBufferCompute> depthsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> PValidsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fValidsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferImage> fIdsImgBuffer;
    std::shared_ptr<LYJ_VK::VKBufferImage> depthsImgBuffer;

    //shader
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comTransV;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comProV;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comTransF;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comProF;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comTransN;
    std::shared_ptr<LYJ_VK::VKCommandTransfer> cmdTransferUVZ;
    std::shared_ptr<LYJ_VK::VKPipelineGraphics> graphDepth;
    std::shared_ptr<LYJ_VK::VKCommandTransfer> cmdTransferDepth;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comRestriveDepth;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comCheckV;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comCheckF;

    //imp
    std::shared_ptr<LYJ_VK::VKImp> impTransV;
    std::shared_ptr<LYJ_VK::VKImp> impProV;
    std::shared_ptr<LYJ_VK::VKImp> impTransF;
    std::shared_ptr<LYJ_VK::VKImp> impProF;
    std::shared_ptr<LYJ_VK::VKImp> impTransN;
    std::shared_ptr<LYJ_VK::VKImp> impTransUVZ;
    std::shared_ptr<LYJ_VK::VKImp> impDepths;
    std::shared_ptr<LYJ_VK::VKImp> impTransDepth;
    std::shared_ptr<LYJ_VK::VKImp> impRestriveDepth;
    std::shared_ptr<LYJ_VK::VKImp> impCheckV;
    std::shared_ptr<LYJ_VK::VKImp> impCheckF;
};


NSP_VULKAN_LYJ_END

#endif