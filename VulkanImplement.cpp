#include "VulkanImplement.h"

#include <mutex>
#include <unordered_map>


NSP_VULKAN_LYJ_BEGIN

namespace {

	std::mutex& commandPoolMutex()
	{
		static std::mutex mutex;
		return mutex;
	}

	std::mutex& queueSubmitMutex(VkQueue queue)
	{
		static std::mutex mapMutex;
		static std::unordered_map<VkQueue, std::shared_ptr<std::mutex>> mutexes;
		std::lock_guard<std::mutex> lock(mapMutex);
		auto& mutex = mutexes[queue];
		if (!mutex)
			mutex.reset(new std::mutex());
		return *mutex;
	}

} // namespace

VKImp::VKImp(VkCommandBufferUsageFlags _cmdUsageFlag)
	:m_usageFlag(_cmdUsageFlag)
{
	m_device = GetLYJVKInstance()->m_device;
	m_commandPool = GetLYJVKInstance()->m_graphicsCommandPool;
	VkCommandBufferAllocateInfo cmdBufferAllocateInfo{};
	cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocateInfo.commandPool = m_commandPool;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandBufferCount = 1;
	std::lock_guard<std::mutex> lock(commandPoolMutex());
	VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &cmdBufferAllocateInfo, &m_commandBuffer));
}
VKImp::~VKImp()
{
	//destroy();
}
inline void VKImp::setCmds(std::vector<VKCommandAbr*> _cmds) { m_cmds = _cmds; }
void VKImp::run(VkQueue _queue, VkFence _fence, std::vector<VkSemaphore> _waitSemaphores, std::vector<VkSemaphore> _signalSemaphores,
	const VkPipelineStageFlags* _waitStageMask)
{
	if (m_needBuild) {
		build();
		m_needBuild = false;
	}
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;
	submitInfo.pWaitDstStageMask = _waitStageMask;
	submitInfo.waitSemaphoreCount = _waitSemaphores.size();
	submitInfo.pWaitSemaphores = _waitSemaphores.data();
	submitInfo.signalSemaphoreCount = _signalSemaphores.size();
	submitInfo.pSignalSemaphores = _signalSemaphores.data();
	std::lock_guard<std::mutex> lock(queueSubmitMutex(_queue));
	VK_CHECK_RESULT(vkQueueSubmit(_queue, 1, &submitInfo, _fence));
}
void VKImp::destroy()
{
	if (m_commandBuffer) {
		std::lock_guard<std::mutex> lock(commandPoolMutex());
		vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
		m_commandBuffer = VK_NULL_HANDLE;
	}
}
bool VKImp::build()
{
	std::lock_guard<std::mutex> lock(commandPoolMutex());
	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = m_usageFlag;
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

VkResult VKImpPresent::present(VkQueue _queue,
	std::vector<VkSwapchainKHR>& _swapChains, std::vector<uint32_t>& _imageIndexs,
	std::vector<VkSemaphore>& _waitSemaphores)
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = _waitSemaphores.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = _swapChains.data();
	presentInfo.pImageIndices = _imageIndexs.data();
	return vkQueuePresentKHR(_queue, &presentInfo);
}







NSP_VULKAN_LYJ_END