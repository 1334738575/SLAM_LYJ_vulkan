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
    uint32_t projectFSize;
    uint32_t projectFStep;
    uint32_t useFaceIds;
    uint32_t projectPSize;
    uint32_t projectPStep;
    uint32_t usePointIds;
    uint32_t cameraModel;
    uint32_t padding0;
    uint32_t padding1;
    float distortion[4];
};

struct UBOProjectGraph
{
    float halfW;
    float halfH;
    float maxD;
    float csTh;
    uint32_t useFaceIds;
    uint32_t usePointIds;
    uint32_t padding1;
    uint32_t padding2;
};


class VULKAN_LYJ_API ProjectorCacheVK
{
public:
    ProjectorCacheVK() {};
    ProjectorCacheVK(unsigned int _PSize, unsigned int _fSize, int _w, int _h, uint32_t _queueIndex = 0);
    ~ProjectorCacheVK();
    void release();

    unsigned int PSize_ = 0;
    unsigned int fSize_ = 0;
    int w_ = 0;
    int h_ = 0;
    uint32_t queueIndex_ = 0;
    bool built_ = false;
    UBOProjectCompute uboComCPU_{};
    UBOProjectGraph uboGraphCPU_{};
    std::vector<uint32_t> fIds_;
    std::vector<float> depths_;
    std::vector<uint32_t> PValids_;
    std::vector<uint32_t> fValids_;
    std::vector<uint32_t> selectedFaceIds_;
    std::vector<uint32_t> selectedIndices_;
    std::vector<uint32_t> selectedPointIds_;
    std::vector<uint32_t> pointMask_;

    std::shared_ptr<VKBufferCompute> TBuffer;
    std::shared_ptr<LYJ_VK::VKBufferUniform> uboCom;
    std::shared_ptr<LYJ_VK::VKBufferUniform> uboGraph;
    std::shared_ptr<LYJ_VK::VKBufferCompute> uvPsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fncsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> uvfcsBuffer;

    std::shared_ptr<LYJ_VK::VKBufferCompute> PValidsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fValidsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> selectedFaceIdsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> selectedPointIdsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> pointMaskBuffer;
    std::shared_ptr<LYJ_VK::VKBufferIndex> selectedIndBuffer;
    std::shared_ptr<LYJ_VK::VKBufferImage> fIdsImgBuffer;
    std::shared_ptr<LYJ_VK::VKBufferImage> depthsImgBuffer;


    uint32_t kernel_ = 1024;
    VkDeviceSize PBufferSize;
    VkDeviceSize fBufferSize;
    std::shared_ptr<VKFence> fence_;
    VkQueue queue;
    VkQueue graphicQueue;
    VkDeviceSize fIdsBufferSize;
    VkDeviceSize PValidsBufferSize;
    VkDeviceSize fValidsBufferSize;
    VkDeviceSize selectedFaceIdsBufferSize;
    VkDeviceSize selectedPointIdsBufferSize;
    VkDeviceSize pointMaskBufferSize;
    VkDeviceSize selectedIndBufferSize;
    uint32_t selectedIndexCount = 0;

    std::vector<std::shared_ptr<VKCommandAbr>> cmdBars;

    //shader
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comTransV;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comTransF;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comTransN;
    std::shared_ptr<LYJ_VK::VKPipelineGraphics> graphDepth;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comCheckV;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comCheckF;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comCheckSelectedV;
    std::shared_ptr<LYJ_VK::VKPipelineCompute> comCheckSelectedF;

    //imp
    std::shared_ptr<LYJ_VK::VKImp> impTransV;
    std::shared_ptr<LYJ_VK::VKImp> impTransF;
    std::shared_ptr<LYJ_VK::VKImp> impTransN;
    std::shared_ptr<LYJ_VK::VKImp> impTransUVZ;
    std::shared_ptr<LYJ_VK::VKImp> impDepths;
    std::shared_ptr<LYJ_VK::VKImp> impDepthToShaderRead;
    std::shared_ptr<LYJ_VK::VKImp> impCheckV;
    std::shared_ptr<LYJ_VK::VKImp> impCheckF;
    std::shared_ptr<LYJ_VK::VKImp> impCheckSelectedV;
    std::shared_ptr<LYJ_VK::VKImp> impCheckSelectedF;
    std::shared_ptr<LYJ_VK::VKImp> impProjectFull;


    void init(unsigned int _PSize, unsigned int _fSize, int _w, int _h, uint32_t _queueIndex = 0);
private:

};

class VULKAN_LYJ_API ProjectorVK
{
public:
    ProjectorVK() {};
    ~ProjectorVK() {};

    bool create(const float* Pws, const unsigned int PSize,
        const float* centers, const float* fNormals, const unsigned int* faces, const unsigned int fSize,
        float* camParams, const int w, const int h, CameraModel cameraModel = CameraModel::Pinhole);

    uint32_t getQueueCount() const;

    void project(ProjectorCacheVK& cache,
        float* Tcw,
        float* depths, unsigned int* fIds, char* allVisiblePIds, char* allVisibleFIds,
        float minD = 0, float maxD = FLT_MAX, float csTh = 0, float detDTh = 1,
        std::vector<uint32_t>* faceIds = nullptr,
        std::vector<uint32_t>* pointIds = nullptr);

    void release();

    unsigned int PSize = 0;
    unsigned int fSize = 0;
    int w_ = 0;
    int h_ = 0;
    UBOProjectCompute uboComCPU_{};
    UBOProjectGraph uboGraphCPU_{};
    uint32_t kernel_ = 1024;
    VkDeviceSize PBufferSize = 0;
    VkDeviceSize fBufferSize = 0;
    std::shared_ptr<LYJ_VK::VKBufferCompute> PwsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fcwsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferCompute> fnsBuffer;
    std::shared_ptr<LYJ_VK::VKBufferIndex> indBuffer;
    std::vector<uint32_t> faces_;

private:
    LYJ_VK::VKInstance* lyjVK = nullptr;
    VkQueue queue = VK_NULL_HANDLE;
    VkQueue graphicQueue = VK_NULL_HANDLE;
    std::shared_ptr<LYJ_VK::VKFence> fence_;

};

NSP_VULKAN_LYJ_END

#endif
