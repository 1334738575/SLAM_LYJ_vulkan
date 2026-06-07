#include "ProjectorVK.h"

#include <future>
#include <chrono>

NSP_VULKAN_LYJ_BEGIN

namespace {

    template <typename T>
    void destroyPtr(std::shared_ptr<T>& ptr)
    {
        if (ptr) {
            ptr->destroy();
            ptr.reset();
        }
    }

    uint32_t getAvailableProjectorQueueCount()
    {
        auto* lyjVK = GetLYJVKInstance();
        const uint32_t computeCount = static_cast<uint32_t>(lyjVK->m_computeQueues.size());
        const uint32_t graphicCount = static_cast<uint32_t>(lyjVK->m_graphicQueues.size());
        if (computeCount == 0 || graphicCount == 0)
            return 0;
        return std::min(computeCount, graphicCount);
    }

    void buildProjectorCache(ProjectorVK& projector, ProjectorCacheVK& cache)
    {
        static std::string vulkanHomePath(VULKAN_LYJ_HOME_PATH);
        static std::string shaderPath = vulkanHomePath + "/shader/";

        if (cache.PSize_ != projector.PSize || cache.fSize_ != projector.fSize ||
            cache.w_ != projector.w_ || cache.h_ != projector.h_ || !cache.TBuffer) {
            cache.init(projector.PSize, projector.fSize, projector.w_, projector.h_, cache.queueIndex_);
        }

        cache.uboComCPU_ = projector.uboComCPU_;
        cache.uboGraphCPU_ = projector.uboGraphCPU_;
        cache.uboCom->upload(sizeof(LYJ_VK::UBOProjectCompute), &cache.uboComCPU_, cache.queue);
        cache.uboGraph->upload(sizeof(LYJ_VK::UBOProjectGraph), &cache.uboGraphCPU_, cache.queue);

        LYJ_VK::ClassResolver classResolver;
        classResolver.addBindingDescriptor(0, 3 * sizeof(float));
        classResolver.addAttributeDescriptor(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

        cache.comTransV.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/transformProjectV.comp.spv"));
        cache.comTransV->setBufferBinding(0, cache.uboCom.get());
        cache.comTransV->setBufferBinding(1, projector.PwsBuffer.get());
        cache.comTransV->setBufferBinding(2, cache.uvPsBuffer.get());
        cache.comTransV->setBufferBinding(3, cache.TBuffer.get());
        cache.comTransV->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comTransV->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierTransV;
        cmdBarrierTransV.reset(new LYJ_VK::VKCommandBufferBarrier({ projector.PwsBuffer->getBuffer(), cache.TBuffer->getBuffer(), cache.uboCom->getBuffer() },
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierTransV);
        cache.impTransV.reset(new LYJ_VK::VKImp(0));
        cache.impTransV->setCmds({ cmdBarrierTransV.get(), cache.comTransV.get() });

        cache.comTransF.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/transformProjectF.comp.spv"));
        cache.comTransF->setBufferBinding(0, cache.uboCom.get());
        cache.comTransF->setBufferBinding(1, projector.fcwsBuffer.get());
        cache.comTransF->setBufferBinding(2, cache.uvfcsBuffer.get());
        cache.comTransF->setBufferBinding(3, cache.TBuffer.get());
        cache.comTransF->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comTransF->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierTransF;
        cmdBarrierTransF.reset(new LYJ_VK::VKCommandBufferBarrier({ projector.fcwsBuffer->getBuffer(), cache.TBuffer->getBuffer(), cache.uboCom->getBuffer() },
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierTransF);
        cache.impTransF.reset(new LYJ_VK::VKImp(0));
        cache.impTransF->setCmds({ cmdBarrierTransF.get(), cache.comTransF.get() });

        cache.comTransN.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/transformN.comp.spv"));
        cache.comTransN->setBufferBinding(0, cache.uboCom.get());
        cache.comTransN->setBufferBinding(1, projector.fnsBuffer.get());
        cache.comTransN->setBufferBinding(2, cache.fncsBuffer.get());
        cache.comTransN->setBufferBinding(3, cache.TBuffer.get());
        cache.comTransN->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comTransN->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierTransN;
        cmdBarrierTransN.reset(new LYJ_VK::VKCommandBufferBarrier({ projector.fnsBuffer->getBuffer(), cache.TBuffer->getBuffer(), cache.uboCom->getBuffer() },
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierTransN);
        cache.impTransN.reset(new LYJ_VK::VKImp(0));
        cache.impTransN->setCmds({ cmdBarrierTransN.get(), cache.comTransN.get() });

        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdTransUVZSrc;
        cmdTransUVZSrc.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.uvPsBuffer->getBuffer() },
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT));
        cache.cmdBars.push_back(cmdTransUVZSrc);
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdTransUVZDst;
        cmdTransUVZDst.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.verBuffer->getBuffer() },
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT));
        cache.cmdBars.push_back(cmdTransUVZDst);
        cache.cmdTransferUVZ.reset(new LYJ_VK::VKCommandTransfer(cache.uvPsBuffer->getBuffer(), cache.verBuffer->getBuffer(), cache.PBufferSize));
        cache.impTransUVZ.reset(new LYJ_VK::VKImp(0));
        cache.impTransUVZ->setCmds({ cmdTransUVZSrc.get(), cache.cmdTransferUVZ.get(), cmdTransUVZDst.get() });

        cache.graphDepth.reset(new LYJ_VK::VKPipelineGraphics(shaderPath + "/texture/depths.vert.spv", shaderPath + "/texture/depths.frag.spv", 1));
        cache.graphDepth->setBufferBinding(0, cache.uboGraph.get());
        cache.graphDepth->setVertexBuffer(cache.verBuffer.get(), projector.PSize, classResolver);
        cache.graphDepth->setIndexBuffer(projector.indBuffer.get(), projector.fSize * 3);
        cache.graphDepth->setImage(0, 0, cache.fIdsImgBuffer);
        cache.graphDepth->setDepthImage(cache.depthsImgBuffer);
        VK_CHECK_RESULT(cache.graphDepth->build());
        std::shared_ptr<LYJ_VK::VKCommandImageBarrier> cmdImgBarrier;
        cmdImgBarrier.reset(new LYJ_VK::VKCommandImageBarrier({ cache.graphDepth->getImage(0, 0)->getImage() },
            { cache.graphDepth->getImage(0, 0)->getSubresource() },
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED));
        cache.cmdBars.push_back(cmdImgBarrier);
        cache.impDepths.reset(new LYJ_VK::VKImp(0));
        cache.impDepths->setCmds({ cache.graphDepth.get(), cmdImgBarrier.get() });

        std::shared_ptr<LYJ_VK::VKCommandImageBarrier> cmdTransDepthSrc;
        cmdTransDepthSrc.reset(new LYJ_VK::VKCommandImageBarrier({ cache.graphDepth->getDepthImage()->getImage() },
            { cache.graphDepth->getDepthImage()->getSubresource() },
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT));
        cache.cmdBars.push_back(cmdTransDepthSrc);
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdTransDepthDst;
        cmdTransDepthDst.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.depthsBuffer->getBuffer() },
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdTransDepthDst);
        std::shared_ptr<LYJ_VK::VKCommandImageBarrier> cmdTransDepthDst2;
        cmdTransDepthDst2.reset(new LYJ_VK::VKCommandImageBarrier({ cache.graphDepth->getDepthImage()->getImage() },
            { cache.graphDepth->getDepthImage()->getSubresource() },
            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT));
        cache.cmdBars.push_back(cmdTransDepthDst2);
        cache.cmdTransferDepth.reset(new LYJ_VK::VKCommandTransfer(cache.graphDepth->getDepthImage()->getImage(), cache.depthsBuffer->getBuffer(),
            VkExtent3D{ (uint32_t)projector.w_, (uint32_t)projector.h_, 1 },
            cache.graphDepth->getDepthImage()->getSubresource()));
        cache.impTransDepth.reset(new LYJ_VK::VKImp(0));
        cache.impTransDepth->setCmds({ cmdTransDepthSrc.get(), cache.cmdTransferDepth.get(), cmdTransDepthDst.get(), cmdTransDepthDst2.get() });

        cache.comRestriveDepth.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/restriveDepth.comp.spv"));
        cache.comRestriveDepth->setBufferBinding(0, cache.uboCom.get());
        cache.comRestriveDepth->setBufferBinding(1, cache.depthsBuffer.get());
        cache.comRestriveDepth->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comRestriveDepth->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierRestriveDepth;
        cmdBarrierRestriveDepth.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.depthsBuffer->getBuffer() },
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierRestriveDepth);
        cache.impRestriveDepth.reset(new LYJ_VK::VKImp(0));
        cache.impRestriveDepth->setCmds({ cmdBarrierRestriveDepth.get(), cache.comRestriveDepth.get() });

        cache.comCheckV.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/checkV2UVZ.comp.spv"));
        cache.comCheckV->setBufferBinding(0, cache.uboCom.get());
        cache.comCheckV->setBufferBinding(1, cache.uvPsBuffer.get());
        cache.comCheckV->setBufferBinding(2, cache.depthsBuffer.get());
        cache.comCheckV->setBufferBinding(3, cache.PValidsBuffer.get());
        cache.comCheckV->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comCheckV->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierCheckV;
        cmdBarrierCheckV.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.uvPsBuffer->getBuffer(), cache.depthsBuffer->getBuffer() },
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierCheckV);
        cache.impCheckV.reset(new LYJ_VK::VKImp(0));
        cache.impCheckV->setCmds({ cmdBarrierCheckV.get(), cache.comCheckV.get() });

        cache.comCheckF.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/checkF2UVZ.comp.spv"));
        cache.comCheckF->setBufferBinding(0, cache.uboCom.get());
        cache.comCheckF->setBufferBinding(1, cache.uvfcsBuffer.get());
        cache.comCheckF->setBufferBinding(2, cache.depthsBuffer.get());
        cache.comCheckF->setBufferBinding(3, cache.PValidsBuffer.get());
        cache.comCheckF->setBufferBinding(4, projector.fsBuffer.get());
        cache.comCheckF->setBufferBinding(5, cache.fValidsBuffer.get());
        cache.comCheckF->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comCheckF->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierCheckF;
        cmdBarrierCheckF.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.uvfcsBuffer->getBuffer(), cache.depthsBuffer->getBuffer(), cache.PValidsBuffer->getBuffer() },
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierCheckF);
        cache.impCheckF.reset(new LYJ_VK::VKImp(0));
        cache.impCheckF->setCmds({ cmdBarrierCheckF.get(), cache.comCheckF.get() });

        cache.impProjectFull.reset(new LYJ_VK::VKImp(0));
        cache.impProjectFull->setCmds({
            cmdBarrierTransV.get(), cache.comTransV.get(),
            cmdBarrierTransN.get(), cache.comTransN.get(),
            cmdBarrierTransF.get(), cache.comTransF.get(),
            cmdTransUVZSrc.get(), cache.cmdTransferUVZ.get(), cmdTransUVZDst.get(),
            cache.graphDepth.get(), cmdImgBarrier.get(),
            cmdTransDepthSrc.get(), cache.cmdTransferDepth.get(), cmdTransDepthDst.get(), cmdTransDepthDst2.get(),
            cmdBarrierRestriveDepth.get(), cache.comRestriveDepth.get(),
            cmdBarrierCheckV.get(), cache.comCheckV.get(),
            cmdBarrierCheckF.get(), cache.comCheckF.get()
            });

        cache.built_ = true;
    }

#define LYJ_VK_PROJECTOR_PROFILE
#ifdef LYJ_VK_PROJECTOR_PROFILE
    class ScopedProjectTimer
    {
    public:
        explicit ScopedProjectTimer(const char* name)
            : name_(name), begin_(std::chrono::steady_clock::now())
        {
        }

        ~ScopedProjectTimer()
        {
            const auto end = std::chrono::steady_clock::now();
            std::cout << name_ << ": " << std::chrono::duration<double, std::milli>(end - begin_).count() << " ms" << std::endl;
        }

    private:
        const char* name_;
        std::chrono::steady_clock::time_point begin_;
    };

#define LYJ_PROFILE_SCOPE(name) ScopedProjectTimer scopedProjectTimer##__LINE__(name)
#else
#define LYJ_PROFILE_SCOPE(name)
#endif

} // namespace

ProjectorCacheVK::ProjectorCacheVK(unsigned int _PSize, unsigned int _fSize, int _w, int _h, uint32_t _queueIndex)
    :PSize_(_PSize), fSize_(_fSize), w_(_w), h_(_h), queueIndex_(_queueIndex)
{
    init(PSize_, fSize_, w_, h_, queueIndex_);
}
ProjectorCacheVK::~ProjectorCacheVK()
{
}
void ProjectorCacheVK::init(unsigned int _PSize, unsigned int _fSize, int _w, int _h, uint32_t _queueIndex)
{
    release();
    PSize_ = _PSize;
    fSize_ = _fSize;
    w_ = _w;
    h_ = _h;
    queueIndex_ = _queueIndex;
    kernel_ = 1024;

    auto* lyjVK = GetLYJVKInstance();
    queue = lyjVK->getComputeQueue(queueIndex_);
    graphicQueue = lyjVK->getGraphicQueue(queueIndex_);
    if (queue == VK_NULL_HANDLE)
        queue = lyjVK->getComputeQueue(0);
    if (graphicQueue == VK_NULL_HANDLE)
        graphicQueue = lyjVK->getGraphicQueue(0);
    fence_.reset(new LYJ_VK::VKFence());

    PBufferSize = PSize_ * 3 * sizeof(float);
    fBufferSize = fSize_ * 3 * sizeof(uint32_t);
    fIdsBufferSize = w_ * h_ * sizeof(uint32_t);
    depthsBufferSize = w_ * h_ * sizeof(float);
    PValidsBufferSize = PSize_ * sizeof(uint32_t);
    fValidsBufferSize = fSize_ * sizeof(uint32_t);

    fIds_.assign(w_ * h_, UINT_MAX);
    depths_.assign(w_ * h_, FLT_MAX);
    PValids_.assign(PSize_, 0);
    fValids_.assign(fSize_, 0);

    TBuffer.reset(new LYJ_VK::VKBufferCompute());
    TBuffer->resize(12 * sizeof(float));
    uboCom.reset(new LYJ_VK::VKBufferUniform());
    uboGraph.reset(new LYJ_VK::VKBufferUniform());

    uvPsBuffer.reset(new LYJ_VK::VKBufferCompute());
    uvPsBuffer->resize(PBufferSize);
    fncsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fncsBuffer->resize(fBufferSize);
    uvfcsBuffer.reset(new LYJ_VK::VKBufferCompute());
    uvfcsBuffer->resize(fBufferSize);
    verBuffer.reset(new LYJ_VK::VKBufferVertex());
    verBuffer->resize(PBufferSize);

    depthsBuffer.reset(new LYJ_VK::VKBufferCompute());
    depthsBuffer->resize(depthsBufferSize);
    PValidsBuffer.reset(new LYJ_VK::VKBufferCompute());
    PValidsBuffer->resize(PValidsBufferSize);
    fValidsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fValidsBuffer->resize(fValidsBufferSize);
    fIdsImgBuffer.reset(new LYJ_VK::VKBufferColorImage(w_, h_, 1, 4, LYJ_VK::VKBufferImage::IMAGEVALUETYPE::UINT32));
    depthsImgBuffer.reset(new LYJ_VK::VKBufferDepthImage(w_, h_));
    built_ = false;
}

void ProjectorCacheVK::release()
{
    if (depthsBuffer) depthsBuffer->releaseBufferCopy();
    if (fIdsImgBuffer) fIdsImgBuffer->releaseBufferCopy();
    if (PValidsBuffer) PValidsBuffer->releaseBufferCopy();
    if (fValidsBuffer) fValidsBuffer->releaseBufferCopy();

    cmdBars.clear();

    destroyPtr(impTransV);
    destroyPtr(impTransF);
    destroyPtr(impTransN);
    destroyPtr(impTransUVZ);
    destroyPtr(impDepths);
    destroyPtr(impTransDepth);
    destroyPtr(impRestriveDepth);
    destroyPtr(impCheckV);
    destroyPtr(impCheckF);
    destroyPtr(impProjectFull);

    destroyPtr(comTransV);
    destroyPtr(comTransF);
    destroyPtr(comTransN);
    destroyPtr(graphDepth);
    destroyPtr(comRestriveDepth);
    destroyPtr(comCheckV);
    destroyPtr(comCheckF);

    destroyPtr(TBuffer);
    destroyPtr(uboCom);
    destroyPtr(uboGraph);
    destroyPtr(verBuffer);
    destroyPtr(uvPsBuffer);
    destroyPtr(fncsBuffer);
    destroyPtr(uvfcsBuffer);
    destroyPtr(depthsBuffer);
    destroyPtr(PValidsBuffer);
    destroyPtr(fValidsBuffer);
    destroyPtr(fIdsImgBuffer);
    destroyPtr(depthsImgBuffer);

    cmdTransferUVZ.reset();
    cmdTransferDepth.reset();
    fence_.reset();
    built_ = false;
}

bool ProjectorVK::create(const float* Pws, const unsigned int _PSize,
    const float* centers, const float* fNormals, const unsigned int* faces, const unsigned int _fSize,
    float* camParams, const int w, const int h)
{
    lyjVK = GetLYJVKInstance();
    if (!lyjVK->isInited())
    {
        if (lyjVK->init(false, nullptr, false) != VK_SUCCESS)
        {
            std::cout << "init vulkan fail!" << std::endl;
            return false;
        }
    }

    PSize = _PSize;
    fSize = _fSize;
    w_ = w;
    h_ = h;

    uboComCPU_.vSize = PSize;
    uboComCPU_.fSize = fSize;
    uboComCPU_.w = w;
    uboComCPU_.h = h;
    uboComCPU_.vStep = (PSize + 1023) / kernel_;
    uboComCPU_.fStep = (fSize + 1023) / kernel_;
    uboComCPU_.fx = camParams[0];
    uboComCPU_.fy = camParams[1];
    uboComCPU_.cx = camParams[2];
    uboComCPU_.cy = camParams[3];
    uboComCPU_.detd = 0.1f;
    uboComCPU_.maxD = 30.0f;
    uboComCPU_.minD = 0.1f;
    uboComCPU_.csTh = 0.0f;
    uboComCPU_.dStep = (w * h) / kernel_;

    uboGraphCPU_.halfW = w / 2.0f;
    uboGraphCPU_.halfH = h / 2.0f;
    uboGraphCPU_.maxD = uboComCPU_.maxD;

    PBufferSize = uboComCPU_.vSize * 3 * sizeof(float);
    fBufferSize = uboComCPU_.fSize * 3 * sizeof(uint32_t);
    fence_.reset(new LYJ_VK::VKFence());
    auto& fence = *fence_;
    queue = lyjVK->getComputeQueue(0);
    graphicQueue = lyjVK->m_graphicQueues[0];

    PwsBuffer.reset(new LYJ_VK::VKBufferCompute());
    PwsBuffer->upload(PBufferSize, (void*)Pws, queue, fence.ptr());
    fence.wait();
    fence.reset();
    fsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fsBuffer->upload(fBufferSize, (void*)faces, queue, fence.ptr());
    fence.wait();
    fence.reset();
    fcwsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fcwsBuffer->upload(fBufferSize, (void*)centers, queue, fence.ptr());
    fence.wait();
    fence.reset();
    fnsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fnsBuffer->upload(fBufferSize, (void*)fNormals, queue, fence.ptr());
    fence.wait();
    fence.reset();
    indBuffer.reset(new LYJ_VK::VKBufferIndex());
    indBuffer->upload(fBufferSize, (void*)faces, graphicQueue, fence.ptr());
    fence.wait();
    fence.reset();

    PwsBuffer->releaseBufferCopy();
    fsBuffer->releaseBufferCopy();
    fcwsBuffer->releaseBufferCopy();
    fnsBuffer->releaseBufferCopy();
    indBuffer->releaseBufferCopy();
    return true;
}

uint32_t ProjectorVK::getQueueCount() const
{
    return getAvailableProjectorQueueCount();
}

void ProjectorVK::project(ProjectorCacheVK& cache, float* Tcw, float* depths, unsigned int* fIds,
    char* allVisiblePIds, char* allVisibleFIds, float minD, float maxD, float csTh, float detDTh)
{
    LYJ_PROFILE_SCOPE("project total");
    if (!cache.built_)
        buildProjectorCache(*this, cache);

    auto& fence = *cache.fence_;
    cache.uboComCPU_.minD = minD;
    cache.uboComCPU_.maxD = maxD;
    cache.uboComCPU_.csTh = csTh;
    cache.uboComCPU_.detd = detDTh;
    cache.uboGraphCPU_.maxD = maxD;

    {
        LYJ_PROFILE_SCOPE("upload/reset");
        cache.TBuffer->upload(12 * sizeof(float), Tcw, cache.queue);
        cache.uboCom->upload(sizeof(LYJ_VK::UBOProjectCompute), &cache.uboComCPU_, cache.queue);
        cache.uboGraph->upload(sizeof(LYJ_VK::UBOProjectGraph), &cache.uboGraphCPU_, cache.queue);
        cache.depthsBuffer->resetData(cache.depthsBufferSize, cache.queue);
        cache.PValidsBuffer->resetData(cache.PValidsBufferSize, cache.queue);
        cache.fValidsBuffer->resetData(cache.fValidsBufferSize, cache.queue, fence.ptr());
        fence.wait();
        fence.reset();
    }

    if (cache.queue == cache.graphicQueue) {
        LYJ_PROFILE_SCOPE("gpu full");
        cache.impProjectFull->run(cache.queue, fence.ptr());
        fence.wait();
        fence.reset();
    }
    else {
        {
            LYJ_PROFILE_SCOPE("gpu transform");
            cache.impTransV->run(cache.queue);
            cache.impTransN->run(cache.queue);
            cache.impTransF->run(cache.queue, fence.ptr());
            fence.wait();
            fence.reset();
        }
        {
            LYJ_PROFILE_SCOPE("gpu uv transfer");
            cache.impTransUVZ->run(cache.queue, fence.ptr());
            fence.wait();
            fence.reset();
        }
        {
            LYJ_PROFILE_SCOPE("gpu depth draw");
            cache.impDepths->run(cache.queue, fence.ptr());
            fence.wait();
            fence.reset();
        }
        {
            LYJ_PROFILE_SCOPE("gpu depth transfer");
            cache.impTransDepth->run(cache.queue, fence.ptr());
            fence.wait();
            fence.reset();
        }
        {
            LYJ_PROFILE_SCOPE("gpu restrict depth");
            cache.impRestriveDepth->run(cache.queue, fence.ptr());
            fence.wait();
            fence.reset();
        }
        {
            LYJ_PROFILE_SCOPE("gpu check");
            cache.impCheckV->run(cache.queue);
            cache.impCheckF->run(cache.queue, fence.ptr());
            fence.wait();
            fence.reset();
        }
    }

    void* datad = nullptr;
    void* dataf = nullptr;
    void* pvalidPtr = nullptr;
    void* fvalidPtr = nullptr;
    {
        LYJ_PROFILE_SCOPE("download submit/wait");
        LYJ_VK::VKFence graphicFence;
        if (depths)
            datad = cache.depthsBuffer->download(cache.depthsBufferSize, cache.graphicQueue, fIds ? nullptr : graphicFence.ptr());
        if (fIds)
            dataf = cache.fIdsImgBuffer->download(cache.fIdsBufferSize, cache.graphicQueue, graphicFence.ptr());
        if (allVisiblePIds)
            pvalidPtr = cache.PValidsBuffer->download(cache.PValidsBufferSize, cache.queue, allVisibleFIds ? nullptr : fence.ptr());
        if (allVisibleFIds)
            fvalidPtr = cache.fValidsBuffer->download(cache.fValidsBufferSize, cache.queue, fence.ptr());
        if (datad || dataf)
            graphicFence.wait();
        if (pvalidPtr || fvalidPtr) {
            fence.wait();
            fence.reset();
        }
        if (datad)
            cache.depthsBuffer->invalidateBufferCopy();
        if (dataf)
            cache.fIdsImgBuffer->invalidateBufferCopy();
        if (pvalidPtr)
            cache.PValidsBuffer->invalidateBufferCopy();
        if (fvalidPtr)
            cache.fValidsBuffer->invalidateBufferCopy();
    }
    {
        LYJ_PROFILE_SCOPE("download memcpy");
        if (datad)
            memcpy(cache.depths_.data(), datad, cache.depthsBufferSize);
        if (dataf)
            memcpy(cache.fIds_.data(), dataf, cache.fIdsBufferSize);
        if (pvalidPtr)
            memcpy(cache.PValids_.data(), pvalidPtr, cache.PValidsBufferSize);
        if (fvalidPtr)
            memcpy(cache.fValids_.data(), fvalidPtr, cache.fValidsBufferSize);
    }

    if (!depths && !fIds && !allVisiblePIds && !allVisibleFIds)
        return;

    const uint32_t sss = cache.uboComCPU_.w * cache.uboComCPU_.h;
    auto copyOutputs = [&](uint64_t s, uint64_t e) {
        for (uint64_t i = s; i < e && i < sss; ++i) {
            if (depths) {
                if (cache.depths_[i] == FLT_MAX || cache.depths_[i] == 0)
                    depths[i] = FLT_MAX;
                else
                    depths[i] = cache.depths_[i];
            }
            if (fIds) {
                if (cache.fIds_[i] == UINT_MAX || cache.fIds_[i] == 0)
                    fIds[i] = UINT_MAX;
                else
                    fIds[i] = cache.fIds_[i] - 1;
            }
        }
        const uint64_t ps = std::min<uint64_t>(s, cache.uboComCPU_.vSize);
        const uint64_t pe = std::min<uint64_t>(e, cache.uboComCPU_.vSize);
        if (allVisiblePIds) {
            for (uint64_t i = ps; i < pe; ++i)
                allVisiblePIds[i] = (char)cache.PValids_[i];
        }
        const uint64_t fs = std::min<uint64_t>(s, cache.uboComCPU_.fSize);
        const uint64_t fe = std::min<uint64_t>(e, cache.uboComCPU_.fSize);
        if (allVisibleFIds) {
            for (uint64_t i = fs; i < fe; ++i)
                allVisibleFIds[i] = (char)cache.fValids_[i];
        }
        };

    uint64_t maxSize = 0;
    if (depths || fIds)
        maxSize = std::max<uint64_t>(maxSize, sss);
    if (allVisiblePIds)
        maxSize = std::max<uint64_t>(maxSize, cache.uboComCPU_.vSize);
    if (allVisibleFIds)
        maxSize = std::max<uint64_t>(maxSize, cache.uboComCPU_.fSize);
    {
        LYJ_PROFILE_SCOPE("cpu output copy");
        const uint64_t threadCount = std::min<uint64_t>(4, std::max<uint64_t>(1, maxSize / (256 * 1024)));
        if (threadCount <= 1) {
            copyOutputs(0, maxSize);
        }
        else {
            std::vector<std::future<void>> tasks;
            tasks.reserve(static_cast<size_t>(threadCount - 1));
            const uint64_t block = (maxSize + threadCount - 1) / threadCount;
            for (uint64_t t = 1; t < threadCount; ++t) {
                const uint64_t s = t * block;
                const uint64_t e = std::min<uint64_t>(maxSize, s + block);
                tasks.emplace_back(std::async(std::launch::async, copyOutputs, s, e));
            }
            copyOutputs(0, std::min<uint64_t>(maxSize, block));
            for (auto& task : tasks)
                task.get();
        }
    }
}

void ProjectorVK::release()
{
    destroyPtr(PwsBuffer);
    destroyPtr(fsBuffer);
    destroyPtr(fcwsBuffer);
    destroyPtr(fnsBuffer);
    destroyPtr(indBuffer);
    fence_.reset();
}

NSP_VULKAN_LYJ_END
