#ifndef VULKAN_COMMAND_H
#define VULKAN_COMMAND_H


#include "VulkanCommon.h"

namespace cv {
	class Mat;
}

NSP_VULKAN_LYJ_BEGIN

class VULKAN_LYJ_API VKCommandAbr
{
public:
	enum CMDTYPE
	{
		DEFAULT,
		BUFFERBARRIER,
		MEMORYBUFFER,
		COMPUTE,
		GRAPHICS,
		TRANSFER
	};
	VKCommandAbr();
	~VKCommandAbr();


	virtual void record(VkCommandBuffer _cmdBuffer)=0;
protected:
	CMDTYPE m_type = CMDTYPE::DEFAULT;
};




class VULKAN_LYJ_API VKCommandBufferBarrier : public VKCommandAbr
{
public:
	VKCommandBufferBarrier(
		std::vector<VkBuffer> _bufferfs,
		VkAccessFlags _srcMask=VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
		VkAccessFlags _dstMask= VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
		VkPipelineStageFlags _srcStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags _dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
		);
	~VKCommandBufferBarrier();

	// 通过 VKCommandAbr 继承
	void record(VkCommandBuffer _cmdBuffer) override;

private:
	std::vector<VkBuffer> m_buffers;
	VkAccessFlags m_srcMask;
	VkAccessFlags m_dstMask;
	VkPipelineStageFlags m_srcStage;
	VkPipelineStageFlags m_dstStage;
	std::vector<VkBufferMemoryBarrier> m_bufferMemoryBarriers;
};

class VULKAN_LYJ_API VKCommandImageBarrier : public VKCommandAbr
{
public:
	VKCommandImageBarrier(
		std::vector<VkImage> _images,
		std::vector<VkImageSubresourceRange> _subresourceRanges,
		VkAccessFlags _srcMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
		VkAccessFlags _dstMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
		VkImageLayout _srcLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageLayout _dstLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkPipelineStageFlags _srcStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags _dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	);
	~VKCommandImageBarrier();
	// 通过 VKCommandAbr 继承
	void record(VkCommandBuffer _cmdBuffer) override;

private:
	std::vector<VkImage> m_images;
	VkAccessFlags m_srcMask;
	VkAccessFlags m_dstMask;
	VkImageLayout m_srcLayout;
	VkImageLayout m_dstLayout;
	VkPipelineStageFlags m_srcStage;
	VkPipelineStageFlags m_dstStage;
	std::vector<VkImageMemoryBarrier> m_imageMemoryBarriers;
	std::vector<VkImageSubresourceRange> m_subresourceRanges;
};


class VULKAN_LYJ_API VKCommandMemoryBarrier : public VKCommandAbr
{
public:
	VKCommandMemoryBarrier(
		VkAccessFlags _srcMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
		VkAccessFlags _dstMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
		VkPipelineStageFlags _srcStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags _dstStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
	);
	~VKCommandMemoryBarrier();


	// 通过 VKCommandAbr 继承
	void record(VkCommandBuffer _cmdBuffer) override;

private:
	VkAccessFlags m_srcMask;
	VkAccessFlags m_dstMask;
	VkPipelineStageFlags m_srcStage;
	VkPipelineStageFlags m_dstStage;
	VkMemoryBarrier m_memoryBarrier{};
};





class VULKAN_LYJ_API VKCommandTransfer : public VKCommandAbr
{
public:
	enum TRANSTYPE
	{
		BUFFER2BUFFER,
		BUFFER2IMAGE,
		IMAGE2BUFFER,
		IMAGE2IMAGE
	};
	VKCommandTransfer(VkBuffer _srcBuffer, VkBuffer _dstBuffer,
		VkDeviceSize _size=VK_WHOLE_SIZE, VkDeviceSize _offsetSrc=0, VkDeviceSize _offsetDst = 0);
	VKCommandTransfer(VkBuffer _srcBuffer, VkImage _dstImage,
		VkExtent3D _size, VkImageSubresourceRange _subResourceRange,
		VkDeviceSize _offsetSrc = 0, VkOffset3D _offsetDst = { 0,0,0 });
	VKCommandTransfer(VkImage _srcImage, VkBuffer _dstBuffer,
		VkExtent3D _size, VkImageSubresourceRange _subResourceRange,
		VkOffset3D _offsetSrc = { 0,0,0 }, VkDeviceSize _offsetDst = 0);
	VKCommandTransfer(VkImage _srcImage, VkImage _dstImage,
		VkExtent3D _size, 
		VkImageSubresourceRange _srcSubResourceRange, VkImageSubresourceRange _dstSubResourceRange,
		VkOffset3D _offsetSrc = { 0,0,0 }, VkOffset3D _offsetDst ={ 0,0,0 });
	~VKCommandTransfer();


	// 通过 VKCommandAbr 继承
	void record(VkCommandBuffer _cmdBuffer) override;
private:
	TRANSTYPE m_transType = BUFFER2BUFFER;

	VkBuffer m_srcBuffer = VK_NULL_HANDLE;
	VkBuffer m_dstBuffer = VK_NULL_HANDLE;
	VkDeviceSize m_size = VK_WHOLE_SIZE;
	VkDeviceSize m_offsetSrc = 0;
	VkDeviceSize m_offsetDst = 0;

	VkImage m_srcImage = VK_NULL_HANDLE;
	VkImageSubresourceRange m_srcSubResourceRange{};
	VkOffset3D m_srcImgOffset{};
	VkImage m_dstImage = VK_NULL_HANDLE;
	VkImageSubresourceRange m_dstSubResourceRange{};
	VkExtent3D m_imgSize{};
	VkOffset3D m_dstImgOffset{};
};








NSP_VULKAN_LYJ_END


#endif // !VULKAN_COMMAND_H
