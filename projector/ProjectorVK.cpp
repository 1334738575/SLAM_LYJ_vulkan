#include "ProjectorVK.h"

NSP_VULKAN_LYJ_BEGIN



ProjectorVK::ProjectorVK()
{
}

ProjectorVK::~ProjectorVK()
{
}

bool ProjectorVK::create(const float* Pws, const unsigned int PSize,
    const float* centers, const float* fNormals, const unsigned int* faces, const unsigned int fSize,
    float* camParams, const int w, const int h)
{
    // init
    lyjVK = GetLYJVKInstance();
    if (!lyjVK->isInited())
    {
        if (lyjVK->init(false, nullptr, true) != VK_SUCCESS)
        {
            std::cout << "init vulkan fail!" << std::endl;
            return false;
        }
    }


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
    uboComCPU_.detd = 0.1;
    uboComCPU_.maxD = 30.0f;
    uboComCPU_.minD = 0.1f;
    uboComCPU_.csTh = 0.0f;
    uboComCPU_.dStep = (w * h) / kernel_;

    uboGraphCPU_.halfW = w / 2.0f;
    uboGraphCPU_.halfH = h / 2.0f;
    uboGraphCPU_.maxD = uboComCPU_.maxD;

    fIds_.resize(uboComCPU_.w * uboComCPU_.h, UINT_MAX);
    depths_.resize(uboComCPU_.w * uboComCPU_.h, FLT_MAX);
    PValids_.resize(uboComCPU_.vSize, 0);
    fValids_.resize(uboComCPU_.fSize, 0);

    // pipeline
    PBufferSize = uboComCPU_.vSize * 3 * sizeof(float);
    fBufferSize = uboComCPU_.fSize * 3 * sizeof(uint32_t);
    fence_.reset(new LYJ_VK::VKFence());
    auto& fence = *fence_;
    queue = lyjVK->getComputeQueue(0);
    graphicQueue = lyjVK->m_graphicQueues[0];
    // in
    PwsBuffer.reset(new LYJ_VK::VKBufferCompute());
    PwsBuffer->upload(PBufferSize, (void*)Pws, queue);
    fsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fsBuffer->upload(fBufferSize, (void*)faces, queue);
    fcwsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fcwsBuffer->upload(fBufferSize, (void*)centers, queue);
    fnsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fnsBuffer->upload(fBufferSize, (void*)fNormals, queue);
    TBuffer.reset(new LYJ_VK::VKBufferCompute());
    TBuffer->resetData(12 * sizeof(float), queue);
    //TBuffer->upload(12 * sizeof(float), T_.data(), queue);
    // cache
    uboCom.reset(new LYJ_VK::VKBufferUniform());
    uboCom->upload(sizeof(LYJ_VK::UBOProjectCompute), &uboComCPU_, queue);
    uboGraph.reset(new LYJ_VK::VKBufferUniform());
    uboGraph->upload(sizeof(LYJ_VK::UBOProjectGraph), &uboGraphCPU_, queue);
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
    verBuffer->upload(PBufferSize, (void*)Pws, graphicQueue);
    indBuffer.reset(new LYJ_VK::VKBufferIndex());
    indBuffer->upload(fBufferSize, (void*)faces, graphicQueue, fence.ptr());
    fence.wait();
    fence.reset();
    LYJ_VK::ClassResolver classResolver;
    classResolver.addBindingDescriptor(0, 3 * sizeof(float));
    classResolver.addAttributeDescriptor(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
    // out
    fIdsBufferSize = w * h * sizeof(uint32_t);
    depthsBufferSize = w * h * sizeof(float);
    PValidsBufferSize = PSize * sizeof(uint32_t);
    fValidsBufferSize = fSize * sizeof(uint32_t);
    depthsBuffer.reset(new LYJ_VK::VKBufferCompute());
    depthsBuffer->resize(depthsBufferSize);
    //depthsBuffer->upload(depthsBufferSize, depths.data(), queue);
    PValidsBuffer.reset(new LYJ_VK::VKBufferCompute());
    PValidsBuffer->resize(PValidsBufferSize);
    //PValidsBuffer->upload(PValidsBufferSize, PValids.data(), queue);
    fValidsBuffer.reset(new LYJ_VK::VKBufferCompute());
    fValidsBuffer->resize(fValidsBufferSize);
    //fValidsBuffer->upload(fValidsBufferSize, fValids.data(), queue, fence.ptr());
    fIdsImgBuffer.reset(new LYJ_VK::VKBufferColorImage(w, h, 1, 4, LYJ_VK::VKBufferImage::IMAGEVALUETYPE::UINT32));
    depthsImgBuffer.reset(new LYJ_VK::VKBufferDepthImage(w, h));

    // shader
    // transform point
    comTransV.reset(new LYJ_VK::VKPipelineCompute("D:/testCmake/pro6/shader/compute/transformV.comp.spv"));
    comTransV->setBufferBinding(0, uboCom.get());
    comTransV->setBufferBinding(1, PwsBuffer.get());
    comTransV->setBufferBinding(2, PcsBuffer.get());
    comTransV->setBufferBinding(3, TBuffer.get());
    comTransV->setRunKernel(kernel_, 1, 1, kernel_, 1, 1);
    VK_CHECK_RESULT(comTransV->build());
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierTransV;
    cmdBarrierTransV.reset( new LYJ_VK::VKCommandBufferBarrier({ PwsBuffer->getBuffer(), TBuffer->getBuffer(), uboCom->getBuffer() },
        VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    );
	cmdBars.push_back(cmdBarrierTransV);
    std::shared_ptr<LYJ_VK::VKCommandMemoryBarrier> endBarrier;
    endBarrier.reset(new LYJ_VK::VKCommandMemoryBarrier());
    cmdBars.push_back(endBarrier);
    impTransV.reset(new LYJ_VK::VKImp(0));
    impTransV->setCmds({ cmdBarrierTransV.get(), comTransV.get(), endBarrier.get()});
    // project point
    comProV.reset(new LYJ_VK::VKPipelineCompute("D:/testCmake/pro6/shader/compute/projectV2UV.comp.spv"));
    comProV->setBufferBinding(0, uboCom.get());
    comProV->setBufferBinding(1, PcsBuffer.get());
    comProV->setBufferBinding(2, uvPsBuffer.get());
    comProV->setRunKernel(kernel_, 1, 1, kernel_, 1, 1);
    VK_CHECK_RESULT(comProV->build());
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierProV;
    cmdBarrierProV.reset(new LYJ_VK::VKCommandBufferBarrier({ PcsBuffer->getBuffer() },
        VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    );
    cmdBars.push_back(cmdBarrierProV);
    impProV.reset(new LYJ_VK::VKImp(0));
    impProV->setCmds({ cmdBarrierProV.get(), comProV.get(), endBarrier.get()});
    // transform face
    comTransF.reset(new LYJ_VK::VKPipelineCompute("D:/testCmake/pro6/shader/compute/transformF.comp.spv"));
    comTransF->setBufferBinding(0, uboCom.get());
    comTransF->setBufferBinding(1, fcwsBuffer.get());
    comTransF->setBufferBinding(2, fccsBuffer.get());
    comTransF->setBufferBinding(3, TBuffer.get());
    comTransF->setRunKernel(kernel_, 1, 1, kernel_, 1, 1);
    VK_CHECK_RESULT(comTransF->build());
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierTransF;
    cmdBarrierTransF.reset(new LYJ_VK::VKCommandBufferBarrier(
        { fcwsBuffer->getBuffer() },
        VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    );
    cmdBars.push_back(cmdBarrierTransF);
    impTransF.reset(new LYJ_VK::VKImp(0));
    impTransF->setCmds({ cmdBarrierTransF.get(), comTransF.get(), endBarrier.get()});
    // project face
    comProF.reset(new LYJ_VK::VKPipelineCompute("D:/testCmake/pro6/shader/compute/projectF2UV.comp.spv"));
    comProF->setBufferBinding(0, uboCom.get());
    comProF->setBufferBinding(1, fccsBuffer.get());
    comProF->setBufferBinding(2, uvfcsBuffer.get());
    comProF->setRunKernel(kernel_, 1, 1, kernel_, 1, 1);
    VK_CHECK_RESULT(comProF->build());
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierProF;
    cmdBarrierProF.reset(new LYJ_VK::VKCommandBufferBarrier(
        { fccsBuffer->getBuffer() },
        VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    );
    cmdBars.push_back(cmdBarrierProF);
    impProF.reset(new LYJ_VK::VKImp(0));
    impProF->setCmds({ cmdBarrierProF.get(), comProF.get(), endBarrier.get()});
    // transform normal
    comTransN.reset(new LYJ_VK::VKPipelineCompute("D:/testCmake/pro6/shader/compute/transformN.comp.spv"));
    comTransN->setBufferBinding(0, uboCom.get());
    comTransN->setBufferBinding(1, fnsBuffer.get());
    comTransN->setBufferBinding(2, fncsBuffer.get());
    comTransN->setBufferBinding(3, TBuffer.get());
    comTransN->setRunKernel(kernel_, 1, 1, kernel_, 1, 1);
    VK_CHECK_RESULT(comTransN->build());
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierTransN;
    cmdBarrierTransN.reset(new LYJ_VK::VKCommandBufferBarrier(
        { fnsBuffer->getBuffer() },
        VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    );
    cmdBars.push_back(cmdBarrierTransN);
    impTransN.reset(new LYJ_VK::VKImp(0));
    impTransN->setCmds({ cmdBarrierTransN.get(), comTransN.get(), endBarrier.get()});

    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdTransUVZSrc;
    cmdTransUVZSrc.reset(new LYJ_VK::VKCommandBufferBarrier(
        { uvPsBuffer->getBuffer() },
        VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
    );
    cmdBars.push_back(cmdTransUVZSrc);
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdTransUVZDst;
    cmdTransUVZDst.reset(new LYJ_VK::VKCommandBufferBarrier(
        { verBuffer->getBuffer() },
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
    );
    cmdBars.push_back(cmdTransUVZDst);
    cmdTransferUVZ.reset(new LYJ_VK::VKCommandTransfer(uvPsBuffer->getBuffer(), verBuffer->getBuffer(), PBufferSize));
    impTransUVZ.reset(new LYJ_VK::VKImp(0));
    impTransUVZ->setCmds({ cmdTransUVZSrc.get(), cmdTransferUVZ.get(), cmdTransUVZDst.get()});

    // render depth
    graphDepth.reset(new LYJ_VK::VKPipelineGraphics("D:/testCmake/pro6/shader/texture/depths.vert.spv", "D:/testCmake/pro6/shader/texture/depths.frag.spv", 1));
    graphDepth->setBufferBinding(0, uboGraph.get());
    graphDepth->setVertexBuffer(verBuffer.get(), PSize, classResolver);
    graphDepth->setIndexBuffer(indBuffer.get(), fSize * 3);
    graphDepth->setImage(0, 0, fIdsImgBuffer);
    graphDepth->setDepthImage(depthsImgBuffer);
    VK_CHECK_RESULT(graphDepth->build());
    std::shared_ptr<LYJ_VK::VKCommandImageBarrier> cmdImgBarrier;
    cmdImgBarrier.reset(new LYJ_VK::VKCommandImageBarrier(
        { graphDepth->getImage(0, 0)->getImage() }, { graphDepth->getImage(0, 0)->getSubresource() },
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED)
    );
    cmdBars.push_back(cmdImgBarrier);
    impDepths.reset(new LYJ_VK::VKImp(0));
    impDepths->setCmds({ graphDepth.get(), cmdImgBarrier.get()});

    // copy depths
    std::shared_ptr<LYJ_VK::VKCommandImageBarrier> cmdTransDepthSrc;
    cmdTransDepthSrc.reset(new LYJ_VK::VKCommandImageBarrier(
        { graphDepth->getDepthImage()->getImage() }, { graphDepth->getDepthImage()->getSubresource() },
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT)
    );
    cmdBars.push_back(cmdTransDepthSrc);
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdTransDepthDst;
    cmdTransDepthDst.reset(new LYJ_VK::VKCommandBufferBarrier(
        { depthsBuffer->getBuffer() },
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
    );
    cmdBars.push_back(cmdTransDepthDst);
    std::shared_ptr<LYJ_VK::VKCommandImageBarrier> cmdTransDepthDst2;
    cmdTransDepthDst2.reset(new LYJ_VK::VKCommandImageBarrier(
        { graphDepth->getDepthImage()->getImage() }, { graphDepth->getDepthImage()->getSubresource() },
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT)
    );
    cmdBars.push_back(cmdTransDepthDst2);
    cmdTransferDepth.reset(new LYJ_VK::VKCommandTransfer(graphDepth->getDepthImage()->getImage(), depthsBuffer->getBuffer(),
        VkExtent3D{ (uint32_t)w, (uint32_t)h , 1 },
        graphDepth->getDepthImage()->getSubresource()));
    impTransDepth.reset(new LYJ_VK::VKImp(0));
    impTransDepth->setCmds({ cmdTransDepthSrc.get(), cmdTransferDepth.get(), cmdTransDepthDst.get(), cmdTransDepthDst2.get()});

    // restrive depth
    comRestriveDepth.reset(new LYJ_VK::VKPipelineCompute("D:/testCmake/pro6/shader/compute/restriveDepth.comp.spv"));
    comRestriveDepth->setBufferBinding(0, uboCom.get());
    comRestriveDepth->setBufferBinding(1, depthsBuffer.get());
    comRestriveDepth->setRunKernel(kernel_, 1, 1, kernel_, 1, 1);
    VK_CHECK_RESULT(comRestriveDepth->build());
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierRestriveDepth;
    cmdBarrierRestriveDepth.reset(new LYJ_VK::VKCommandBufferBarrier(
        { depthsBuffer->getBuffer() },
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    );
    cmdBars.push_back(cmdBarrierRestriveDepth);
    impRestriveDepth.reset(new LYJ_VK::VKImp(0));
    impRestriveDepth->setCmds({ cmdBarrierRestriveDepth.get(), comRestriveDepth.get(), endBarrier.get()});

    // check point
    comCheckV.reset(new LYJ_VK::VKPipelineCompute("D:/testCmake/pro6/shader/compute/checkV2UVZ.comp.spv"));
    comCheckV->setBufferBinding(0, uboCom.get());
    comCheckV->setBufferBinding(1, uvPsBuffer.get());
    comCheckV->setBufferBinding(2, depthsBuffer.get());
    comCheckV->setBufferBinding(3, PValidsBuffer.get());
    comCheckV->setRunKernel(kernel_, 1, 1, kernel_, 1, 1);
    VK_CHECK_RESULT(comCheckV->build());
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierCheckV;
    cmdBarrierCheckV.reset(new LYJ_VK::VKCommandBufferBarrier(
        { uvPsBuffer->getBuffer(), depthsBuffer->getBuffer() },
        VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    );
    cmdBars.push_back(cmdBarrierCheckV);
    impCheckV.reset(new LYJ_VK::VKImp(0));
    impCheckV->setCmds({ cmdBarrierCheckV.get(), comCheckV.get(), endBarrier.get()});
    // check face
    comCheckF.reset(new LYJ_VK::VKPipelineCompute("D:/testCmake/pro6/shader/compute/checkF2UVZ.comp.spv"));
    comCheckF->setBufferBinding(0, uboCom.get());
    comCheckF->setBufferBinding(1, uvfcsBuffer.get());
    comCheckF->setBufferBinding(2, depthsBuffer.get());
    comCheckF->setBufferBinding(3, PValidsBuffer.get());
    comCheckF->setBufferBinding(4, fsBuffer.get());
    comCheckF->setBufferBinding(5, fValidsBuffer.get());
    comCheckF->setRunKernel(kernel_, 1, 1, kernel_, 1, 1);
    VK_CHECK_RESULT(comCheckF->build());
    std::shared_ptr<LYJ_VK::VKCommandBufferBarrier> cmdBarrierCheckF;
    cmdBarrierCheckF.reset(new LYJ_VK::VKCommandBufferBarrier(
        { uvfcsBuffer->getBuffer(), depthsBuffer->getBuffer(), PValidsBuffer->getBuffer() },
        VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
    );
    cmdBars.push_back(cmdBarrierCheckF);
    impCheckF.reset(new LYJ_VK::VKImp(0));
    impCheckF->setCmds({ cmdBarrierCheckF.get(), comCheckF.get(), endBarrier.get()});


    return true;
}

void ProjectorVK::project(float* Tcw, float* depths, unsigned int* fIds, char* allVisiblePIds, char* allVisibleFIds, float minD, float maxD, float csTh, float detDTh)
{
    auto& fence = *fence_;
    uboComCPU_.minD = minD;
    uboComCPU_.maxD = maxD;
    uboComCPU_.csTh = csTh;
    uboComCPU_.detd = detDTh;


    TBuffer->upload(12 * sizeof(float), Tcw, queue);
    uboCom->upload(sizeof(LYJ_VK::UBOProjectCompute), &uboComCPU_, queue);
    uboGraph->upload(sizeof(LYJ_VK::UBOProjectGraph), &uboGraphCPU_, queue);
    depthsBuffer->resetData(depthsBufferSize, queue);
    PValidsBuffer->resetData(PValidsBufferSize, queue);
    fValidsBuffer->resetData(fValidsBufferSize, queue, fence.ptr());
    fence.wait();
    fence.reset();


    // run1
    impTransV->run(queue);
    impTransN->run(queue);
    impTransF->run(queue, fence.ptr());
    fence.wait();
    fence.reset();
    impProV->run(queue);
    impProF->run(queue, fence.ptr());
    fence.wait();
    fence.reset();

    impTransUVZ->run(queue, fence.ptr());
    fence.wait();
    fence.reset();

    // run2
    impDepths->run(queue, fence.ptr());
    fence.wait();
    fence.reset();

    impTransDepth->run(queue, fence.ptr());
    fence.wait();
    fence.reset();

    impRestriveDepth->run(queue, fence.ptr());
    fence.wait();
    fence.reset();

    //run3
    impCheckV->run(queue);
    impCheckF->run(queue, fence.ptr());
    fence.wait();
    fence.reset();

    //download3
    void* datad = depthsBuffer->download(depthsBufferSize, graphicQueue);
    void* dataf = fIdsImgBuffer->download(fIdsBufferSize, graphicQueue);
    void* pvalidPtr = PValidsBuffer->download(PValidsBufferSize, queue);
    void* fvalidPtr = fValidsBuffer->download(fValidsBufferSize, queue, fence.ptr());
    fence.wait();
    fence.reset();
    memcpy(depths_.data(), datad, depthsBufferSize);
    memcpy(fIds_.data(), dataf, fIdsBufferSize);
    memcpy(PValids_.data(), pvalidPtr, PValidsBufferSize);
    memcpy(fValids_.data(), fvalidPtr, fValidsBufferSize);
    depthsBuffer->releaseBufferCopy();
    fIdsImgBuffer->releaseBufferCopy();
    PValidsBuffer->releaseBufferCopy();
    fValidsBuffer->releaseBufferCopy();

    uint32_t sss = uboComCPU_.w * uboComCPU_.h;
    for (int i = 0; i < sss; ++i) {
        if (depths_[i] == FLT_MAX || depths_[i] == 0)
            depths[i] = FLT_MAX;
        else
            depths[i] = depths_[i];
    }
    for (int i = 0; i < sss; ++i) {
        if (fIds_[i] == UINT_MAX || fIds_[i] == 0)
            fIds[i] = UINT_MAX;
        else
            fIds[i] = fIds_[i] - 1;
    }
    for (int i = 0; i < uboComCPU_.vSize; ++i) {
        allVisiblePIds[i] = (char)PValids_[i];
    }
    for (int i = 0; i < uboComCPU_.fSize; ++i) {
        allVisibleFIds[i] = (char)fValids_[i];
    }
}

void ProjectorVK::release()
{
    // free
    cmdBars.clear();

    PwsBuffer->destroy();
    fsBuffer->destroy();
    fcwsBuffer->destroy();
    fnsBuffer->destroy();
    TBuffer->destroy();
    uboCom->destroy();
    uboGraph->destroy();
    verBuffer->destroy();
    indBuffer->destroy();
    PcsBuffer->destroy();
    uvPsBuffer->destroy();
    fccsBuffer->destroy();
    uvfcsBuffer->destroy();
    depthsBuffer->destroy();
    PValidsBuffer->destroy();
    fValidsBuffer->destroy();
    fIdsImgBuffer->destroy();
    depthsImgBuffer->destroy();

    comTransV->destroy();
    comTransF->destroy();
    comTransN->destroy();
    comProV->destroy();
    comProF->destroy();
    graphDepth->destroy();
    comCheckV->destroy();
    comCheckF->destroy();

    impTransV->destroy();
    impTransF->destroy();
    impTransN->destroy();
    impProV->destroy();
    impProF->destroy();
    impDepths->destroy();
    impCheckV->destroy();
    impCheckF->destroy();
}

NSP_VULKAN_LYJ_END