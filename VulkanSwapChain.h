#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include "VulkanCommon.h"
#include "VulkanBuffer.h"

NSP_VULKAN_LYJ_BEGIN

class VULKAN_LYJ_API VKSwapChain
{
public:
	VKSwapChain()=delete;
	VKSwapChain(uint32_t _imageCnt);
	~VKSwapChain();
	inline VkSwapchainKHR getSwapChain() { return m_swapChain; };
	inline std::vector<VkImage>& getVkImages() { return m_images; };
	inline std::vector<std::shared_ptr<VKBufferImage>>& getImages() { return m_imagePtrs; };
	inline std::vector<VkImageView>& getImageViews() { return m_imageViews; };
	inline const uint32_t getImageCnt() const { return m_imageCnt; };
	inline const VkFormat getFormat() const { return m_format; };
	inline const VkExtent2D& getExtent2D() const { return m_extent; };
	inline const uint32_t& getImageDeviceSize() const { return m_imageDeviceSize; };
	void destory();

	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
			if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				//if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		return availableFormats[0];
	};
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availablePresentMode;
		return VK_PRESENT_MODE_FIFO_KHR;
	};
	static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* _windows)
	{
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(_windows, &width, &height);
			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			return actualExtent;
		}
	};

private:

private:
	VkDevice m_device = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> m_images;
	std::vector<std::shared_ptr<VKBufferImage>> m_imagePtrs;
	std::vector<VkImageView> m_imageViews;
	uint32_t m_imageCnt = 0;
	VkFormat m_format;
	VkExtent2D m_extent;
	uint32_t m_imageDeviceSize = 0;
};




NSP_VULKAN_LYJ_END


#endif // !VULKAN_SWAPCHAIN_H
