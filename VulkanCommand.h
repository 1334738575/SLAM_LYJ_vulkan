#ifndef VULKAN_COMMAND_H
#define VULKAN_COMMAND_H


#include "VulkanCommon.h"


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
		VkAccessFlags _dstMask=VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_READ_BIT,
		VkPipelineStageFlags _srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags _dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
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




class VULKAN_LYJ_API VKCommandMemoryBarrier : public VKCommandAbr
{
public:
	VKCommandMemoryBarrier(
		VkAccessFlags _srcMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
		VkAccessFlags _dstMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_READ_BIT,
		VkPipelineStageFlags _srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags _dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
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
	VKCommandTransfer(VkBuffer _srcBuffer, VkBuffer _dstBuffer,
		VkDeviceSize _size=VK_WHOLE_SIZE, VkDeviceSize _offsetSrc=0, VkDeviceSize _offsetDst = 0);
	~VKCommandTransfer();


	// 通过 VKCommandAbr 继承
	void record(VkCommandBuffer _cmdBuffer) override;
private:
	VkBuffer m_srcBuffer = VK_NULL_HANDLE;
	VkBuffer m_dstBuffer = VK_NULL_HANDLE;
	VkDeviceSize m_size = VK_WHOLE_SIZE;
	VkDeviceSize m_offsetSrc = 0;
	VkDeviceSize m_offsetDst = 0;
};








NSP_VULKAN_LYJ_END


#endif // !VULKAN_COMMAND_H
