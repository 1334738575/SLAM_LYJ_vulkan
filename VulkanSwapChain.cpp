#include "VulkanSwapChain.h"


NSP_VULKAN_LYJ_BEGIN


VKSwapChain::VKSwapChain(uint32_t _imageCnt)
	:m_imgageCnt(_imageCnt)
{
	m_device = GetLYJVKInstance()->m_device;
	LYJ_VK::SwapChainSupportDetails& details = GetLYJVKInstance()->m_details;
	GLFWwindow* windows = GetLYJVKInstance()->m_windows;
	VkSurfaceKHR surface = GetLYJVKInstance()->m_surface;
	LYJ_VK::QueueFamilyIndices& queueIndices = GetLYJVKInstance()->m_queueIndices;

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
	m_format = surfaceFormat.format;
	VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
	m_extent = chooseSwapExtent(details.capabilities, windows);
	m_imgageCnt = details.capabilities.minImageCount + 1;
	if (details.capabilities.maxImageCount > 0 && m_imgageCnt > details.capabilities.maxImageCount) {
		m_imgageCnt = details.capabilities.maxImageCount;
	}
	m_imgageCnt = 2;
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = m_imgageCnt;
	createInfo.imageFormat = m_format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = m_extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	uint32_t queueFamilyIndices[] = { queueIndices.graphicsFamily.value(), queueIndices.presentFamily.value() };
	if (queueIndices.graphicsFamily != queueIndices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}
	createInfo.preTransform = details.capabilities.currentTransform; //������ʾͼ��任����ת������
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //ͼ��ϳ�Ч����͸����
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE; //�ü�����������Ĳ���
	createInfo.oldSwapchain = VK_NULL_HANDLE; //��Ϊ��ʱ�����ھɽ����������ϴ����µ�
	if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain");
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imgageCnt, nullptr);
	m_images.resize(m_imgageCnt);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imgageCnt, m_images.data());

	m_imageViews.resize(m_imgageCnt);
	for (size_t i = 0; i < m_imgageCnt; ++i) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(m_device, &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create image views");
	}
}
VKSwapChain::~VKSwapChain()
{
	//destory();
}
void VKSwapChain::destory()
{
	for (auto imageView : m_imageViews)
		vkDestroyImageView(m_device, imageView, nullptr);
	//for (auto image : m_images)
	//	vkDestroyImage(m_device, image, nullptr);
	vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
}



NSP_VULKAN_LYJ_END
