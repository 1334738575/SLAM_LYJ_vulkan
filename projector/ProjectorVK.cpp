#include "ProjectorVK.h"

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

        std::shared_ptr<LYJ_VK::VKCommandMemoryBarrier> endBarrier;
        endBarrier.reset(new LYJ_VK::VKCommandMemoryBarrier());
        cache.cmdBars.push_back(endBarrier);

        cache.comTransV.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/transformV.comp.spv"));
        cache.comTransV->setBufferBinding(0, cache.uboCom.get());
        cache.comTransV->setBufferBinding(1, projector.PwsBuffer.get());
        cache.comTransV->setBufferBinding(2, cache.PcsBuffer.get());
        cache.comTransV->setBufferBinding(3, cache.TBuffer.get());
        cache.comTransV->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comTransV->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierTransV;
        cmdBarrierTransV.reset(new LYJ_VK::VKCommandBufferBarrier({ projector.PwsBuffer->getBuffer(), cache.TBuffer->getBuffer(), cache.uboCom->getBuffer() },
            VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierTransV);
        cache.impTransV.reset(new LYJ_VK::VKImp(0));
        cache.impTransV->setCmds({ cmdBarrierTransV.get(), cache.comTransV.get(), endBarrier.get() });

        cache.comProV.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/projectV2UV.comp.spv"));
        cache.comProV->setBufferBinding(0, cache.uboCom.get());
        cache.comProV->setBufferBinding(1, cache.PcsBuffer.get());
        cache.comProV->setBufferBinding(2, cache.uvPsBuffer.get());
        cache.comProV->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comProV->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierProV;
        cmdBarrierProV.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.PcsBuffer->getBuffer() },
            VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierProV);
        cache.impProV.reset(new LYJ_VK::VKImp(0));
        cache.impProV->setCmds({ cmdBarrierProV.get(), cache.comProV.get(), endBarrier.get() });

        cache.comTransF.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/transformF.comp.spv"));
        cache.comTransF->setBufferBinding(0, cache.uboCom.get());
        cache.comTransF->setBufferBinding(1, projector.fcwsBuffer.get());
        cache.comTransF->setBufferBinding(2, cache.fccsBuffer.get());
        cache.comTransF->setBufferBinding(3, cache.TBuffer.get());
        cache.comTransF->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comTransF->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierTransF;
        cmdBarrierTransF.reset(new LYJ_VK::VKCommandBufferBarrier({ projector.fcwsBuffer->getBuffer() },
            VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierTransF);
        cache.impTransF.reset(new LYJ_VK::VKImp(0));
        cache.impTransF->setCmds({ cmdBarrierTransF.get(), cache.comTransF.get(), endBarrier.get() });

        cache.comProF.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/projectF2UV.comp.spv"));
        cache.comProF->setBufferBinding(0, cache.uboCom.get());
        cache.comProF->setBufferBinding(1, cache.fccsBuffer.get());
        cache.comProF->setBufferBinding(2, cache.uvfcsBuffer.get());
        cache.comProF->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comProF->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierProF;
        cmdBarrierProF.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.fccsBuffer->getBuffer() },
            VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierProF);
        cache.impProF.reset(new LYJ_VK::VKImp(0));
        cache.impProF->setCmds({ cmdBarrierProF.get(), cache.comProF.get(), endBarrier.get() });

        cache.comTransN.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/transformN.comp.spv"));
        cache.comTransN->setBufferBinding(0, cache.uboCom.get());
        cache.comTransN->setBufferBinding(1, projector.fnsBuffer.get());
        cache.comTransN->setBufferBinding(2, cache.fncsBuffer.get());
        cache.comTransN->setBufferBinding(3, cache.TBuffer.get());
        cache.comTransN->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comTransN->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierTransN;
        cmdBarrierTransN.reset(new LYJ_VK::VKCommandBufferBarrier({ projector.fnsBuffer->getBuffer() },
            VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierTransN);
        cache.impTransN.reset(new LYJ_VK::VKImp(0));
        cache.impTransN->setCmds({ cmdBarrierTransN.get(), cache.comTransN.get(), endBarrier.get() });

        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdTransUVZSrc;
        cmdTransUVZSrc.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.uvPsBuffer->getBuffer() },
            VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT));
        cache.cmdBars.push_back(cmdTransUVZSrc);
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdTransUVZDst;
        cmdTransUVZDst.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.verBuffer->getBuffer() },
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT));
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
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT));
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
        cache.impRestriveDepth->setCmds({ cmdBarrierRestriveDepth.get(), cache.comRestriveDepth.get(), endBarrier.get() });

        cache.comCheckV.reset(new LYJ_VK::VKPipelineCompute(shaderPath + "/compute/checkV2UVZ.comp.spv"));
        cache.comCheckV->setBufferBinding(0, cache.uboCom.get());
        cache.comCheckV->setBufferBinding(1, cache.uvPsBuffer.get());
        cache.comCheckV->setBufferBinding(2, cache.depthsBuffer.get());
        cache.comCheckV->setBufferBinding(3, cache.PValidsBuffer.get());
        cache.comCheckV->setRunKernel(cache.kernel_, 1, 1, cache.kernel_, 1, 1);
        VK_CHECK_RESULT(cache.comCheckV->build());
        std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierCheckV;
        cmdBarrierCheckV.reset(new LYJ_VK::VKCommandBufferBarrier({ cache.uvPsBuffer->getBuffer(), cache.depthsBuffer->getBuffer() },
            VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierCheckV);
        cache.impCheckV.reset(new LYJ_VK::VKImp(0));
        cache.impCheckV->setCmds({ cmdBarrierCheckV.get(), cache.comCheckV.get(), endBarrier.get() });

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
            VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
        cache.cmdBars.push_back(cmdBarrierCheckF);
        cache.impCheckF.reset(new LYJ_VK::VKImp(0));
        cache.impCheckF->setCmds({ cmdBarrierCheckF.get(), cache.comCheckF.get(), endBarrier.get() });

        cache.impProjectFull.reset(new LYJ_VK::VKImp(0));
        cache.impProjectFull->setCmds({
            cmdBarrierTransV.get(), cache.comTransV.get(), endBarrier.get(),
            cmdBarrierTransN.get(), cache.comTransN.get(), endBarrier.get(),
            cmdBarrierTransF.get(), cache.comTransF.get(), endBarrier.get(),
            cmdBarrierProV.get(), cache.comProV.get(), endBarrier.get(),
            cmdBarrierProF.get(), cache.comProF.get(), endBarrier.get(),
            cmdTransUVZSrc.get(), cache.cmdTransferUVZ.get(), cmdTransUVZDst.get(),
            cache.graphDepth.get(), cmdImgBarrier.get(),
            cmdTransDepthSrc.get(), cache.cmdTransferDepth.get(), cmdTransDepthDst.get(), cmdTransDepthDst2.get(),
            cmdBarrierRestriveDepth.get(), cache.comRestriveDepth.get(), endBarrier.get(),
            cmdBarrierCheckV.get(), cache.comCheckV.get(), endBarrier.get(),
            cmdBarrierCheckF.get(), cache.comCheckF.get(),
            endBarrier.get()
            });

        cache.built_ = true;
    }

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

    PcsBuffer.reset(new LYJ_VK::VKBufferCompute());
    PcsBuffer->resize(PBufferSize);
    uvPsBuffer.reset(new LYJ_VK::VKBufferCompute());
    uvPsBuffer->resize(PBufferSize);
    fccsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fccsBuffer->resize(fBufferSize);
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
    destroyPtr(impProV);
    destroyPtr(impProF);
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
    destroyPtr(comProV);
    destroyPtr(comProF);
    destroyPtr(graphDepth);
    destroyPtr(comRestriveDepth);
    destroyPtr(comCheckV);
    destroyPtr(comCheckF);

    destroyPtr(TBuffer);
    destroyPtr(uboCom);
    destroyPtr(uboGraph);
    destroyPtr(verBuffer);
    destroyPtr(PcsBuffer);
    destroyPtr(uvPsBuffer);
    destroyPtr(fccsBuffer);
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
    if (!cache.built_)
        buildProjectorCache(*this, cache);

    auto& fence = *cache.fence_;
    cache.uboComCPU_.minD = minD;
    cache.uboComCPU_.maxD = maxD;
    cache.uboComCPU_.csTh = csTh;
    cache.uboComCPU_.detd = detDTh;
    cache.uboGraphCPU_.maxD = maxD;

    cache.TBuffer->upload(12 * sizeof(float), Tcw, cache.queue);
    cache.uboCom->upload(sizeof(LYJ_VK::UBOProjectCompute), &cache.uboComCPU_, cache.queue);
    cache.uboGraph->upload(sizeof(LYJ_VK::UBOProjectGraph), &cache.uboGraphCPU_, cache.queue);
    cache.depthsBuffer->resetData(cache.depthsBufferSize, cache.queue);
    cache.PValidsBuffer->resetData(cache.PValidsBufferSize, cache.queue);
    cache.fValidsBuffer->resetData(cache.fValidsBufferSize, cache.queue, fence.ptr());
    fence.wait();
    fence.reset();

    if (cache.queue == cache.graphicQueue) {
        cache.impProjectFull->run(cache.queue, fence.ptr());
        fence.wait();
        fence.reset();
    }
    else {
        cache.impTransV->run(cache.queue);
        cache.impTransN->run(cache.queue);
        cache.impTransF->run(cache.queue, fence.ptr());
        fence.wait();
        fence.reset();
        cache.impProV->run(cache.queue);
        cache.impProF->run(cache.queue, fence.ptr());
        fence.wait();
        fence.reset();

        cache.impTransUVZ->run(cache.queue, fence.ptr());
        fence.wait();
        fence.reset();

        cache.impDepths->run(cache.queue, fence.ptr());
        fence.wait();
        fence.reset();

        cache.impTransDepth->run(cache.queue, fence.ptr());
        fence.wait();
        fence.reset();

        cache.impRestriveDepth->run(cache.queue, fence.ptr());
        fence.wait();
        fence.reset();

        cache.impCheckV->run(cache.queue);
        cache.impCheckF->run(cache.queue, fence.ptr());
        fence.wait();
        fence.reset();
    }

    void* datad = cache.depthsBuffer->download(cache.depthsBufferSize, cache.graphicQueue);
    void* dataf = cache.fIdsImgBuffer->download(cache.fIdsBufferSize, cache.graphicQueue);
    void* pvalidPtr = cache.PValidsBuffer->download(cache.PValidsBufferSize, cache.queue);
    void* fvalidPtr = cache.fValidsBuffer->download(cache.fValidsBufferSize, cache.queue, fence.ptr());
    fence.wait();
    fence.reset();
    memcpy(cache.depths_.data(), datad, cache.depthsBufferSize);
    memcpy(cache.fIds_.data(), dataf, cache.fIdsBufferSize);
    memcpy(cache.PValids_.data(), pvalidPtr, cache.PValidsBufferSize);
    memcpy(cache.fValids_.data(), fvalidPtr, cache.fValidsBufferSize);

    const uint32_t sss = cache.uboComCPU_.w * cache.uboComCPU_.h;
    auto copyOutputs = [&](uint64_t s, uint64_t e) {
        for (uint64_t i = s; i < e && i < sss; ++i) {
            if (cache.depths_[i] == FLT_MAX || cache.depths_[i] == 0)
                depths[i] = FLT_MAX;
            else
                depths[i] = cache.depths_[i];
            if (cache.fIds_[i] == UINT_MAX || cache.fIds_[i] == 0)
                fIds[i] = UINT_MAX;
            else
                fIds[i] = cache.fIds_[i] - 1;
        }
        const uint64_t ps = std::min<uint64_t>(s, cache.uboComCPU_.vSize);
        const uint64_t pe = std::min<uint64_t>(e, cache.uboComCPU_.vSize);
        for (uint64_t i = ps; i < pe; ++i)
            allVisiblePIds[i] = (char)cache.PValids_[i];
        const uint64_t fs = std::min<uint64_t>(s, cache.uboComCPU_.fSize);
        const uint64_t fe = std::min<uint64_t>(e, cache.uboComCPU_.fSize);
        for (uint64_t i = fs; i < fe; ++i)
            allVisibleFIds[i] = (char)cache.fValids_[i];
        };

    const uint64_t maxSize = std::max<uint64_t>(sss, std::max<uint64_t>(cache.uboComCPU_.vSize, cache.uboComCPU_.fSize));
    copyOutputs(0, maxSize);
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
