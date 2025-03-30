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
        m_bufferMemoryBarriers[i].buffer = m_buffers[i]; // Ŀ�껺����
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
		m_imageMemoryBarriers[i].srcAccessMask = m_srcMask; // Դ��������
		m_imageMemoryBarriers[i].dstAccessMask = m_dstMask; // Ŀ���������
		m_imageMemoryBarriers[i].oldLayout = m_srcLayout; // Դͼ�񲼾�
		m_imageMemoryBarriers[i].newLayout = m_dstLayout; // Ŀ��ͼ�񲼾�
		m_imageMemoryBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		m_imageMemoryBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		m_imageMemoryBarriers[i].image = _images[i]; // Ŀ��ͼ��
        m_imageMemoryBarriers[i].subresourceRange = m_subresourceRanges[i]; // ͼ����
  //      m_imageMemoryBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // ͼ����
		//m_imageMemoryBarriers[i].subresourceRange.baseMipLevel = 0; // ��һ��mipmap�㼶
		//m_imageMemoryBarriers[i].subresourceRange.levelCount = 1; // mipmap�㼶��
		//m_imageMemoryBarriers[i].subresourceRange.baseArrayLayer = 0; // ��һ������㼶
		//m_imageMemoryBarriers[i].subresourceRange.layerCount = 1; // ����㼶��
	}
}
VKCommandImageBarrier::~VKCommandImageBarrier()
{}
void VKCommandImageBarrier::record(VkCommandBuffer _cmdBuffer)
{
	vkCmdPipelineBarrier(
		_cmdBuffer, // �������
		m_srcStage, // Դ���߽׶�
		m_dstStage, // Ŀ����߽׶�
		0, // �����ѡ��
		0, nullptr, // û�������ڴ�����
		0, nullptr, // û�������������ڴ�����
		m_images.size(), &m_imageMemoryBarriers[0] // �������ǵ�ͼ���ڴ�����
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
	m_transType = TRANSTYPE::BUFFER2BUFFER;
}
VKCommandTransfer::VKCommandTransfer(VkBuffer _srcBuffer, VkImage _dstImage,
    VkExtent3D _size, VkImageSubresourceRange _subResourceRange,
    VkDeviceSize _offsetSrc, VkOffset3D _offsetDst)
	:m_srcBuffer(_srcBuffer), m_dstImage(_dstImage), m_imgSize(_size), m_dstSubResourceRange(_subResourceRange), m_offsetSrc(_offsetSrc), m_dstImgOffset(_offsetDst)
{
    m_type = CMDTYPE::TRANSFER;
    m_transType = TRANSTYPE::BUFFER2IMAGE;
}
VKCommandTransfer::VKCommandTransfer(VkImage _srcImage, VkBuffer _dstBuffer,
    VkExtent3D _size, VkImageSubresourceRange _subResourceRange,
    VkOffset3D _offsetSrc, VkDeviceSize _offsetDst)
    :m_srcImage(_srcImage), m_dstBuffer(_dstBuffer), m_imgSize(_size), m_srcSubResourceRange(_subResourceRange), m_srcImgOffset(_offsetSrc), m_offsetDst(_offsetDst)
{
    m_type = CMDTYPE::TRANSFER;
    m_transType = TRANSTYPE::IMAGE2BUFFER;
}
VKCommandTransfer::VKCommandTransfer(VkImage _srcImage, VkImage _dstImage,
    VkExtent3D _size,
    VkImageSubresourceRange _srcSubResourceRange, VkImageSubresourceRange _dstSubResourceRange,
    VkOffset3D _offsetSrc, VkOffset3D _offsetDst)
    :m_srcImage(_srcImage), m_dstImage(_dstImage), m_imgSize(_size), m_srcSubResourceRange(_srcSubResourceRange), m_dstSubResourceRange(_dstSubResourceRange), m_srcImgOffset(_offsetSrc), m_dstImgOffset(_offsetDst)
{
    m_type = CMDTYPE::TRANSFER;
    m_transType = TRANSTYPE::IMAGE2IMAGE;
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
        bufferImageCopy.imageOffset = m_dstImgOffset;
        bufferImageCopy.imageExtent = m_imgSize;
        vkCmdCopyBufferToImage(_cmdBuffer, m_srcBuffer, m_dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);
        break;
    }
    case TRANSTYPE::IMAGE2BUFFER: {
        VkBufferImageCopy imageBufferCopy{};
        imageBufferCopy.bufferOffset = m_offsetDst;
        imageBufferCopy.bufferRowLength = 0;//meas row length in texels
        imageBufferCopy.bufferImageHeight = 0;
        imageBufferCopy.imageSubresource.aspectMask = m_srcSubResourceRange.aspectMask;
        imageBufferCopy.imageSubresource.mipLevel = m_srcSubResourceRange.baseMipLevel;
        imageBufferCopy.imageSubresource.baseArrayLayer = m_srcSubResourceRange.baseArrayLayer;
        imageBufferCopy.imageSubresource.layerCount = m_srcSubResourceRange.layerCount;
        imageBufferCopy.imageOffset = m_srcImgOffset;
        imageBufferCopy.imageExtent = m_imgSize;
        vkCmdCopyImageToBuffer(_cmdBuffer, m_srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_dstBuffer, 1, &imageBufferCopy);
        break;
    }
    case TRANSTYPE::IMAGE2IMAGE: {
        VkImageCopy imageCopy{};
        imageCopy.extent = m_imgSize;
        imageCopy.srcOffset = m_srcImgOffset;
        imageCopy.dstOffset = m_dstImgOffset;
        imageCopy.srcSubresource.aspectMask = m_srcSubResourceRange.aspectMask;
        imageCopy.srcSubresource.baseArrayLayer = m_srcSubResourceRange.baseArrayLayer;
        imageCopy.srcSubresource.layerCount = m_srcSubResourceRange.layerCount;
        imageCopy.srcSubresource.mipLevel = m_srcSubResourceRange.baseMipLevel;
        imageCopy.dstSubresource.aspectMask = m_dstSubResourceRange.aspectMask;
        imageCopy.dstSubresource.baseArrayLayer = m_dstSubResourceRange.baseArrayLayer;
        imageCopy.dstSubresource.layerCount = m_dstSubResourceRange.layerCount;
        imageCopy.dstSubresource.mipLevel = m_dstSubResourceRange.baseMipLevel;
        vkCmdCopyImage(_cmdBuffer, m_srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        break;
    }
    default:
        break;
    }
}


NSP_VULKAN_LYJ_END

