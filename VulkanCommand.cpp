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
        m_bufferMemoryBarriers[i].srcAccessMask = m_srcMask; // Դ��������
        m_bufferMemoryBarriers[i].dstAccessMask = m_dstMask; // Ŀ���������
        m_bufferMemoryBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        m_bufferMemoryBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        m_bufferMemoryBarriers[i].buffer = _bufferfs[i]; // Ŀ�껺����
        m_bufferMemoryBarriers[i].offset = 0; // �ӻ������Ŀ�ʼ��
        m_bufferMemoryBarriers[i].size = VK_WHOLE_SIZE; // ����������
    }
}
VKCommandBufferBarrier::~VKCommandBufferBarrier()
{}
void VKCommandBufferBarrier::record(VkCommandBuffer _cmdBuffer)
{
    vkCmdPipelineBarrier(
        _cmdBuffer, // �������
        m_srcStage, // Դ���߽׶�
        m_dstStage, // Ŀ����߽׶�
        0, // �����ѡ��
        0, nullptr, // û�������ڴ�����
        m_bufferMemoryBarriers.size(), &m_bufferMemoryBarriers[0], // �������ǵĻ������ڴ�����
        0, nullptr // û����������
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
    m_memoryBarrier.srcAccessMask = m_srcMask; // Դ��������
    m_memoryBarrier.dstAccessMask = m_dstMask; // Ŀ���������
}
VKCommandMemoryBarrier::~VKCommandMemoryBarrier()
{}
void VKCommandMemoryBarrier::record(VkCommandBuffer _cmdBuffer)
{
    vkCmdPipelineBarrier(
        _cmdBuffer, // �������
        m_srcStage, // Դ���߽׶�
        m_dstStage, // Ŀ����߽׶�
        0, // �����ѡ��
        1, &m_memoryBarrier, // �������ǵ��ڴ�����
        0, nullptr, // û�������ڴ�����
        0, nullptr // û����������
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


