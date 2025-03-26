#include "VulkanImplement.h"


NSP_VULKAN_LYJ_BEGIN

VKImp::VKImp(VkCommandBufferUsageFlags _cmdUsageFlag)
	:m_usageFlag(_cmdUsageFlag)
{
	m_device = GetLYJVKInstance()->m_device;
	m_commandPool = GetLYJVKInstance()->m_graphicsCommandPool;
	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{};
	cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocateInfo.commandPool = m_commandPool; //一般图形都满足计算
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandBufferCount = 1;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &cmdBufferAllocateInfo, &m_commandBuffer));
}
VKImp::~VKImp()
{
	vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
}
inline void VKImp::setCmds(std::vector<VKCommandAbr*> _cmds) { m_cmds = _cmds; }
void VKImp::run(VkQueue _queue, VkFence _fence, std::vector<VkSemaphore> _waitSemaphores, std::vector<VkSemaphore> _signalSemaphores,
	const VkPipelineStageFlags* _waitStageMask)
{
	build();
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;
	submitInfo.pWaitDstStageMask = _waitStageMask;
	submitInfo.waitSemaphoreCount = _waitSemaphores.size();
	submitInfo.pWaitSemaphores = _waitSemaphores.data();
	submitInfo.signalSemaphoreCount = _signalSemaphores.size();
	submitInfo.pSignalSemaphores = _signalSemaphores.data();
	VK_CHECK_RESULT(vkQueueSubmit(_queue, 1, &submitInfo, _fence));
}
bool VKImp::build()
{
	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffer, &cmdBufferBeginInfo));
	for (auto& cmd : m_cmds)
	{
		cmd->record(m_commandBuffer);
	}
	VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffer));
	return true;
}




VKImpPresent::VKImpPresent()
{
}
VKImpPresent::~VKImpPresent()
{
}







NSP_VULKAN_LYJ_END