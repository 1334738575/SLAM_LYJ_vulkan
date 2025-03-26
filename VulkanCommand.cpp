#include "VulkanCommand.h"


NSP_VULKAN_LYJ_BEGIN

VKCommandAbr::VKCommandAbr()
{}
VKCommandAbr::~VKCommandAbr()
{}



VKCommandBufferBarrier::VKCommandBufferBarrier(
    std::vector<VkBuffer> _bufferfs,
    VkAccessFlags _srcMask, VkAccessFlags _dstMask,
    VkPipelineStageFlags _srcStage, VkPipelineStageFlags _dstStage
    )
	:m_buffers(_bufferfs), m_srcMask(_srcMask), m_dstMask(_dstMask), m_srcStage(_srcStage), m_dstStage(_dstStage)
{
	m_type = CMDTYPE::BUFFERBARRIER;
	m_bufferMemoryBarriers.resize(m_buffers.size());
    for (int i = 0; i < m_buffers.size(); ++i) {
        m_bufferMemoryBarriers[i] = VkBufferMemoryBarrier{};
        m_bufferMemoryBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        m_bufferMemoryBarriers[i].pNext = nullptr;
        m_bufferMemoryBarriers[i].srcAccessMask = m_srcMask; // 源访问掩码
        m_bufferMemoryBarriers[i].dstAccessMask = m_dstMask; // 目标访问掩码
        m_bufferMemoryBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        m_bufferMemoryBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        m_bufferMemoryBarriers[i].buffer = _bufferfs[i]; // 目标缓冲区
        m_bufferMemoryBarriers[i].offset = 0; // 从缓冲区的开始处
        m_bufferMemoryBarriers[i].size = VK_WHOLE_SIZE; // 整个缓冲区
    }
}
VKCommandBufferBarrier::~VKCommandBufferBarrier()
{}
void VKCommandBufferBarrier::record(VkCommandBuffer _cmdBuffer)
{
    vkCmdPipelineBarrier(
        _cmdBuffer, // 命令缓冲区
        m_srcStage, // 源管线阶段
        m_dstStage, // 目标管线阶段
        0, // 额外的选项
        0, nullptr, // 没有其他内存屏障
        m_bufferMemoryBarriers.size(), &m_bufferMemoryBarriers[0], // 传入我们的缓冲区内存屏障
        0, nullptr // 没有其他屏障
    );

}



VKCommandMemoryBarrier::VKCommandMemoryBarrier(
    VkAccessFlags _srcMask, VkAccessFlags _dstMask,
	VkPipelineStageFlags _srcStage, VkPipelineStageFlags _dstStage
    )
	:m_dstMask(_dstMask), m_srcMask(_srcMask), m_srcStage(_srcStage), m_dstStage(_dstStage)
{
	m_type = CMDTYPE::MEMORYBUFFER;
    m_memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    m_memoryBarrier.pNext = nullptr;
    m_memoryBarrier.srcAccessMask = m_srcMask; // 源访问掩码
    m_memoryBarrier.dstAccessMask = m_dstMask; // 目标访问掩码
}
VKCommandMemoryBarrier::~VKCommandMemoryBarrier()
{}
void VKCommandMemoryBarrier::record(VkCommandBuffer _cmdBuffer)
{
    vkCmdPipelineBarrier(
        _cmdBuffer, // 命令缓冲区
        m_srcStage, // 源管线阶段
        m_dstStage, // 目标管线阶段
        0, // 额外的选项
        1, &m_memoryBarrier, // 传入我们的内存屏障
        0, nullptr, // 没有其他内存屏障
        0, nullptr // 没有其他屏障
    );
}




VKCommandTransfer::VKCommandTransfer(VkBuffer _srcBuffer, VkBuffer _dstBuffer,
    VkDeviceSize _size, VkDeviceSize _offsetSrc, VkDeviceSize _offsetDst)
	:m_srcBuffer(_srcBuffer), m_dstBuffer(_dstBuffer), m_size(_size), m_offsetSrc(_offsetSrc), m_offsetDst(_offsetDst)
{
	m_type = CMDTYPE::TRANSFER;
}
VKCommandTransfer::~VKCommandTransfer()
{}
void VKCommandTransfer::record(VkCommandBuffer _cmdBuffer)
{
	VkBufferCopy bufferCopy{};
	bufferCopy.size = m_size;
    bufferCopy.srcOffset = m_offsetSrc;
    bufferCopy.dstOffset = m_offsetDst;
	vkCmdCopyBuffer(_cmdBuffer, m_srcBuffer, m_dstBuffer, 1, &bufferCopy);
}


NSP_VULKAN_LYJ_END


