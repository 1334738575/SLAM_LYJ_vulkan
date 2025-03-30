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
        m_bufferMemoryBarriers[i].buffer = m_buffers[i]; // 目标缓冲区
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




VKCommandImageBarrier::VKCommandImageBarrier(std::vector<VkImage> _images,
    std::vector<VkImageSubresourceRange> _subresourceRanges,
    VkAccessFlags _srcMask, VkAccessFlags _dstMask,
    VkImageLayout _srcLayout, VkImageLayout _dstLayout,
    VkPipelineStageFlags _srcStage, VkPipelineStageFlags _dstStage)
	:m_images(_images), m_subresourceRanges(_subresourceRanges),
    m_srcMask(_srcMask), m_dstMask(_dstMask),
    m_srcLayout(_srcLayout), m_dstLayout(_dstLayout),
    m_srcStage(_srcStage), m_dstStage(_dstStage)
{
	m_type = CMDTYPE::BUFFERBARRIER;
	m_imageMemoryBarriers.resize(m_images.size());
	for (int i = 0; i < m_images.size(); ++i) {
		m_imageMemoryBarriers[i] = VkImageMemoryBarrier{};
		m_imageMemoryBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		m_imageMemoryBarriers[i].pNext = nullptr;
		m_imageMemoryBarriers[i].srcAccessMask = m_srcMask; // 源访问掩码
		m_imageMemoryBarriers[i].dstAccessMask = m_dstMask; // 目标访问掩码
		m_imageMemoryBarriers[i].oldLayout = m_srcLayout; // 源图像布局
		m_imageMemoryBarriers[i].newLayout = m_dstLayout; // 目标图像布局
		m_imageMemoryBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		m_imageMemoryBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		m_imageMemoryBarriers[i].image = _images[i]; // 目标图像
        m_imageMemoryBarriers[i].subresourceRange = m_subresourceRanges[i]; // 图像方面
  //      m_imageMemoryBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // 图像方面
		//m_imageMemoryBarriers[i].subresourceRange.baseMipLevel = 0; // 第一个mipmap层级
		//m_imageMemoryBarriers[i].subresourceRange.levelCount = 1; // mipmap层级数
		//m_imageMemoryBarriers[i].subresourceRange.baseArrayLayer = 0; // 第一个数组层级
		//m_imageMemoryBarriers[i].subresourceRange.layerCount = 1; // 数组层级数
	}
}
VKCommandImageBarrier::~VKCommandImageBarrier()
{}
void VKCommandImageBarrier::record(VkCommandBuffer _cmdBuffer)
{
	vkCmdPipelineBarrier(
		_cmdBuffer, // 命令缓冲区
		m_srcStage, // 源管线阶段
		m_dstStage, // 目标管线阶段
		0, // 额外的选项
		0, nullptr, // 没有其他内存屏障
		0, nullptr, // 没有其他缓冲区内存屏障
		m_images.size(), &m_imageMemoryBarriers[0] // 传入我们的图像内存屏障
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
	m_transType = TRANSTYPE::BUFFER2BUFFER;
}
VKCommandTransfer::VKCommandTransfer(VkBuffer _srcBuffer, VkImage _dstImage,
    VkExtent3D _size, VkImageSubresourceRange _subResourceRange,
    VkDeviceSize _offsetSrc, VkOffset3D _offsetDst)
	:m_srcBuffer(_srcBuffer), m_dstImage(_dstImage), m_imgSize(_size), m_dstSubResourceRange(_subResourceRange), m_offsetSrc(_offsetSrc), m_imgOffset(_offsetDst)
{
    m_type = CMDTYPE::TRANSFER;
    m_transType = TRANSTYPE::BUFFER2IMAGE;
}
VKCommandTransfer::~VKCommandTransfer()
{}
void VKCommandTransfer::record(VkCommandBuffer _cmdBuffer)
{
    switch (m_transType)
    {
    case TRANSTYPE::BUFFER2BUFFER: {
        VkBufferCopy bufferCopy{};
        bufferCopy.size = m_size;
        bufferCopy.srcOffset = m_offsetSrc;
        bufferCopy.dstOffset = m_offsetDst;
        vkCmdCopyBuffer(_cmdBuffer, m_srcBuffer, m_dstBuffer, 1, &bufferCopy);
        break;
    }
    case TRANSTYPE::BUFFER2IMAGE: {
        VkBufferImageCopy bufferImageCopy{};
        bufferImageCopy.bufferOffset = m_offsetSrc;
		bufferImageCopy.bufferRowLength = 0;//meas row length in texels
        bufferImageCopy.bufferImageHeight = 0;
        bufferImageCopy.imageSubresource.aspectMask = m_dstSubResourceRange.aspectMask;
        bufferImageCopy.imageSubresource.mipLevel = m_dstSubResourceRange.baseMipLevel;
        bufferImageCopy.imageSubresource.baseArrayLayer = m_dstSubResourceRange.baseArrayLayer;
        bufferImageCopy.imageSubresource.layerCount = m_dstSubResourceRange.layerCount;
        bufferImageCopy.imageOffset = m_imgOffset;
        bufferImageCopy.imageExtent = m_imgSize;
        vkCmdCopyBufferToImage(_cmdBuffer, m_srcBuffer, m_dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);
        break;
    }
	case TRANSTYPE::IMAGE2BUFFER:
		break;
	case TRANSTYPE::IMAGE2IMAGE:
		break;
    default:
        break;
    }
}


NSP_VULKAN_LYJ_END

