#ifndef VULKAN_IMPLEMENT_H
#define VULKAN_IMPLEMENT_H

#include "VulkanCommon.h"
#include "VulkanCommand.h"
#include "VulkanPipeline.h"


NSP_VULKAN_LYJ_BEGIN


class VULKAN_LYJ_API VKImp
{
public:
	VKImp(VkCommandBufferUsageFlags _cmdUsageFlag);
	~VKImp();

	void setCmds(std::vector<VKCommandAbr*> _cmds);
	void run(VkQueue _queue,
		VkFence _fence=nullptr,
		std::vector<VkSemaphore> _waitSemaphores= std::vector<VkSemaphore>(),
		std::vector<VkSemaphore> _signalSemaphores= std::vector<VkSemaphore>(),
		const VkPipelineStageFlags* _waitStageMask= nullptr);
	void destroy();
private:
	bool build();

private:
	std::vector<VKCommandAbr*> m_cmds;
	VkCommandBufferUsageFlags m_usageFlag=0;
	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	bool m_needBuild = true;
};





class VULKAN_LYJ_API VKImpPresent
{
public:
	VKImpPresent();
	~VKImpPresent();

	VkResult present(VkQueue _queue,
		std::vector<VkSwapchainKHR>& _swapChains, std::vector<uint32_t>& _imageIndexs,
		std::vector<VkSemaphore>& _waitSemaphores);
private:
	//std::vector<VkSwapchainKHR> m_swapChains;
};




NSP_VULKAN_LYJ_END


#endif // !VULKAN_IMPLEMENT_H
