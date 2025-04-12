#include "VulkanSwapChain.h"


NSP_VULKAN_LYJ_BEGIN


VKSwapChain::VKSwapChain(uint32_t _imageCnt)
	:m_imageCnt(_imageCnt)
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
	//m_imageCnt = details.capabilities.minImageCount + 1;
	//if (details.capabilities.maxImageCount > 0 && m_imageCnt > details.capabilities.maxImageCount) {
	//	m_imageCnt = details.capabilities.maxImageCount;
	//}
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = m_imageCnt;
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
	createInfo.preTransform = details.capabilities.currentTransform; //最终显示图像变换（旋转，镜像）
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //图像合成效果，透明度
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE; //裁剪超出窗口外的部分
	createInfo.oldSwapchain = VK_NULL_HANDLE; //不为空时，将在旧交换链基础上创建新的
	if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
		throw std::runtime_error("failed to create swap chain");
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imageCnt, nullptr);
	m_images.resize(m_imageCnt);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imageCnt, m_images.data());

	m_imageViews.resize(m_imageCnt);
	for (size_t i = 0; i < m_imageCnt; ++i) {
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

	int w = m_extent.width;
	int h = m_extent.height;
	int c = 4;
	int step = 1;
	m_imageDeviceSize = w * h * c * step;
	m_imagePtrs.resize(m_imageCnt, nullptr);
	for (int i = 0; i < m_imageCnt; ++i)
		m_imagePtrs[i].reset(new LYJ_VK::VKBufferColorImage(m_images[i], m_imageViews[i], w, h, c, step, VKBufferImage::IMAGEVALUETYPE::UINT8));
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
