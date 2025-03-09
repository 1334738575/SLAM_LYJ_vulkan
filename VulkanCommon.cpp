#include "VulkanCommon.h"

NSP_VULKAN_LYJ_BEGIN


Instance::Instance()
{
}
Instance::~Instance()
{
	clean();
}


VkResult Instance::init(bool _bGlfw, bool _bValid)
{
	m_init = false;
	m_bGlfw = _bGlfw;
	m_bValid = _bValid;
	m_enableInstanceExtensions.clear();
	m_enableLayers.clear();

	VkResult ret = createInstance();
	if (ret != VK_SUCCESS) {
		m_init = false;
		return ret;
	}

	ret = createPhysicalDevice();
	if (ret != VK_SUCCESS) {
		m_init = false;
		return ret;
	}

	ret = createDeviceAndQueue();
	if (ret != VK_SUCCESS) {
		m_init = false;
		return ret;
	}

	ret = createCommandPool();
	if (ret != VK_SUCCESS) {
		m_init = false;
		return ret;
	}

	m_init = true;
	return ret;
}
void Instance::clean()
{
	vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
	vkDestroyCommandPool(m_device, m_presentCommandPool, nullptr);
	vkDestroyCommandPool(m_device, m_computeCommandPool, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

VkResult Instance::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vlukan LYJ";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> neededInstanceExtensions;
	if (m_bGlfw)
	{
		neededInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		neededInstanceExtensions.push_back("VK_KHR_win32_surface");
	}
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			for (VkExtensionProperties& extension : extensions)
				m_supportInstanceExtensions.push_back(extension.extensionName);
	}
	if (m_enableInstanceExtensions.size() > 0)
	{
		for (const char* enabledExtension : m_enableInstanceExtensions)
		{
			// Output message if requested extension is not available
			if (std::find(m_supportInstanceExtensions.begin(), m_supportInstanceExtensions.end(), enabledExtension) == m_supportInstanceExtensions.end())
				std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
			m_enableInstanceExtensions.push_back(enabledExtension);
		}
	}
	if (m_bValid || std::find(m_supportInstanceExtensions.begin(), m_supportInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != m_supportInstanceExtensions.end()) {
		m_enableInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	if (m_enableInstanceExtensions.size() > 0) {
		createInfo.enabledExtensionCount = (uint32_t)m_enableInstanceExtensions.size();
		createInfo.ppEnabledExtensionNames = m_enableInstanceExtensions.data();
	}

	std::vector<const char*> enableLayers = { "VK_LAYER_KHRONOS_validation" };
	if (m_bValid) {
		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
		bool validationLayerPresent = false;
		for (VkLayerProperties& layer : instanceLayerProperties) {
			m_supportLayers.push_back(layer.layerName);
			if (strcmp(layer.layerName, enableLayers[0]) == 0) {
				validationLayerPresent = true;
				break;
			}
		}
		if (validationLayerPresent) {
			createInfo.ppEnabledLayerNames = &enableLayers[0];
			createInfo.enabledLayerCount = 1;
		}
		else {
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}
	return(vkCreateInstance(&createInfo, nullptr, &m_instance));
}
VkResult Instance::createPhysicalDevice()
{
	auto funcFindQueueFamilies = [](VkPhysicalDevice device, std::vector<VkQueueFamilyProperties>& queueFamilies, VkSurfaceKHR surface=nullptr) -> QueueFamilyIndices {
		QueueFamilyIndices queueIndices;
		uint32_t queueFamilyCnt = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCnt, nullptr);
		queueFamilies.resize(queueFamilyCnt);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCnt, queueFamilies.data());
		bool bFind = false;
		int i = 0;
		VkBool32 presentSupport = false;
		for (const auto& queueFamily : queueFamilies) {
			if ( (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !queueIndices.isCompleteGraphic())
				queueIndices.graphicsFamily = i;
			if (surface) {
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
				if (presentSupport && !queueIndices.isCompletePresent())
					queueIndices.presentFamily = i;
			}
			if ( (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !queueIndices.isCompleteCompute())
				queueIndices.computeFamily = i;
			++i;
		}
		return queueIndices;
		};
	auto funcQuerySwapChainSupport = [](VkPhysicalDevice device, VkSurfaceKHR surface) -> SwapChainSupportDetails {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
		uint32_t formatCnt;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCnt, nullptr);
		if (formatCnt != 0) {
			details.formats.resize(formatCnt);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCnt, details.formats.data());
		}
		uint32_t presentModeCnt;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCnt, nullptr);
		if (presentModeCnt != 0) {
			details.presentModes.resize(presentModeCnt);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCnt, nullptr);
		}
		return details;
		};
	auto funcDeviceSuitable = [&](VkPhysicalDevice device) {
		std::vector<VkQueueFamilyProperties> queueFamilies;
		QueueFamilyIndices indices = funcFindQueueFamilies(device, queueFamilies, m_surface);
		uint32_t extensionCnt = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCnt, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCnt);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCnt, availableExtensions.data());
		std::set<std::string> requiredExtensions(m_enableDeviceExtensions.begin(), m_enableDeviceExtensions.end());
		for (const auto& extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);
		bool extensionsSupported = requiredExtensions.empty();
		bool swapChainAdequate = false;
		SwapChainSupportDetails details;
		if (extensionsSupported) {
			details = funcQuerySwapChainSupport(device, m_surface);
			swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
		}
		VkPhysicalDeviceMemoryProperties memProperties{};
		VkPhysicalDeviceFeatures devFeature{};
		VkPhysicalDeviceProperties devProperties{};
		vkGetPhysicalDeviceProperties(device, &devProperties);
		vkGetPhysicalDeviceFeatures(device, &devFeature);
		vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

		bool ret = devProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && devFeature.geometryShader\
			&& indices.isCompleteGraphic() && indices.isCompletePresent() && extensionsSupported && swapChainAdequate \
			&& indices.isCompleteCompute();

		if (ret) {
			m_queueFamilies = queueFamilies;
			m_queueIndices = indices;
			for (const auto& extension : availableExtensions)
				m_supportDeviceExtensions.push_back(extension.extensionName);
			m_details = details;
			m_memProperties = memProperties;
			m_devFeature = devFeature;
			m_devProperties = devProperties;
		}
		return ret;
		};
	uint32_t deviceCnt = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCnt, nullptr);
	if (deviceCnt == 0) {
		std::cout << "failed to find GPUs" << std::endl;
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	std::vector<VkPhysicalDevice> devices(deviceCnt);
	vkEnumeratePhysicalDevices(m_instance, &deviceCnt, devices.data());
	m_enableDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	for (const auto& device : devices) {
		if (funcDeviceSuitable(device)) {
			m_physicalDevice = device;
			break;
		}
	}
	if (m_physicalDevice == VK_NULL_HANDLE) 
		return VK_ERROR_INITIALIZATION_FAILED;
	return VK_SUCCESS;
}
VkResult Instance::createDeviceAndQueue()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFailies = { m_queueIndices.graphicsFamily.value(), m_queueIndices.presentFamily.value(), m_queueIndices.computeFamily.value() };
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFailies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = m_queueFamilies[queueFamily].queueCount;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &m_devFeature;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_enableDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_enableDeviceExtensions.data();
	createInfo.enabledLayerCount = 0;
	VkResult ret = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);

	if (m_queueIndices.isCompleteGraphic()) {
		uint32_t fi = m_queueIndices.graphicsFamily.value();
		int qCnt = m_queueFamilies[fi].queueCount;
		m_graphicQueues.resize(qCnt);
		for (int i = 0; i < qCnt; ++i) 
			vkGetDeviceQueue(m_device, fi, i, &m_graphicQueues[i]);
	}
	if (m_queueIndices.isCompleteGraphic()) {
		uint32_t fi = m_queueIndices.presentFamily.value();
		if (fi == m_queueIndices.graphicsFamily.value()) {
			m_presentQueues = m_graphicQueues;
		}
		else {
			int qCnt = m_queueFamilies[fi].queueCount;
			m_presentQueues.resize(qCnt);
			for (int i = 0; i < qCnt; ++i)
				vkGetDeviceQueue(m_device, fi, i, &m_presentQueues[i]);
		}
	}
	if (m_queueIndices.isCompleteCompute()) {
		uint32_t fi = m_queueIndices.computeFamily.value();
		if (fi == m_queueIndices.graphicsFamily.value()) {
			m_computeQueues = m_graphicQueues;
		}
		else if (m_queueIndices.presentFamily.value() == fi) {
			m_computeQueues = m_presentQueues;
		}
		else {
			int qCnt = m_queueFamilies[fi].queueCount;
			m_computeQueues.resize(qCnt);
			for (int i = 0; i < qCnt; ++i)
				vkGetDeviceQueue(m_device, fi, i, &m_computeQueues[i]);
		}
	}
	return ret;
}
VkResult Instance::createCommandPool()
{
	VkCommandPoolCreateInfo cmdPoolCreateInfo{};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	cmdPoolCreateInfo.queueFamilyIndex = m_queueIndices.graphicsFamily.value();
	VkResult ret = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_graphicsCommandPool);
	if (ret != VK_SUCCESS)
		return ret;
	cmdPoolCreateInfo.queueFamilyIndex = m_queueIndices.presentFamily.value();
	ret = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_presentCommandPool);
	if (ret != VK_SUCCESS)
		return ret;
	cmdPoolCreateInfo.queueFamilyIndex = m_queueIndices.computeFamily.value();
	ret = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_computeCommandPool);
	return ret;
}

NSP_VULKAN_LYJ_END


