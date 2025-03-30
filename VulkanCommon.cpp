#include "VulkanCommon.h"

NSP_VULKAN_LYJ_BEGIN


VKInstance::VKInstance()
{
}
VKInstance::~VKInstance()
{
	//clean();
}


bool VKInstance::isInited() {
	return m_init;
}
VkResult VKInstance::init(bool _bGlfw, GLFWwindow* _windows, bool _bValid)
{
	VkResult ret = VK_SUCCESS;
	m_init = false;
	m_bGlfw = _bGlfw;
	m_bValid = _bValid;
	m_enableInstanceExtensions.clear();
	m_enableLayers.clear();

	auto funcCreateWinows = [&](int _w, int _h) {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_windows = glfwCreateWindow(_w, _h, "Vulkan", nullptr, nullptr);
		};
	if (m_bGlfw) {
		if(_windows == nullptr)
			funcCreateWinows(m_width, m_height);
		else
			m_windows = _windows;
	}

	ret = createInstance();
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
void VKInstance::clean()
{
	if (m_queueIndices.isCompleteGraphic()) {
		vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
	}
	if (m_queueIndices.isCompletePresent() &&
		m_queueIndices.presentFamily.value() != m_queueIndices.graphicsFamily.value()) {
		vkDestroyCommandPool(m_device, m_presentCommandPool, nullptr);
	}
	if (m_queueIndices.isCompleteCompute() &&
		m_queueIndices.computeFamily.value() != m_queueIndices.presentFamily.value() &&
		m_queueIndices.computeFamily.value() != m_queueIndices.graphicsFamily.value()) {
		vkDestroyCommandPool(m_device, m_computeCommandPool, nullptr);
	}
	vkDestroyDevice(m_device, nullptr);
	if (m_bGlfw) {
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	}
	vkDestroyInstance(m_instance, nullptr);
	if (m_bGlfw) {
		glfwDestroyWindow(m_windows);
		glfwTerminate();
	}
}
VkQueue VKInstance::getGraphicQueue(int _i)
{
	if (_i >= m_graphicQueues.size())
		return VK_NULL_HANDLE;
	return m_graphicQueues[_i];
}
VkQueue VKInstance::getPresentQueue(int _i)
{
	if (_i >= m_presentQueues.size())
		return VK_NULL_HANDLE;
	return m_presentQueues[_i];
}
VkQueue VKInstance::getComputeQueue(int _i)
{
	if (_i >= m_computeQueues.size())
		return VK_NULL_HANDLE;
	return m_computeQueues[_i];
}
uint32_t VKInstance::getMemoryTypeIndex(uint32_t _typeBits, VkMemoryPropertyFlags _properties)
{
	for (uint32_t i = 0; i < m_memProperties.memoryTypeCount; ++i) {
		if ((_typeBits & 1) == 1)
			if ((m_memProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
				return i;
		_typeBits >>= 1;
	}
	return 0;
}
VkResult VKInstance::createInstance()
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
		neededInstanceExtensions.push_back("VK_KHR_win32_surface");
		neededInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
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
	if (neededInstanceExtensions.size() > 0)
	{
		for (const char* enabledExtension : neededInstanceExtensions)
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
VkResult VKInstance::createPhysicalDevice()
{
	auto createSurface = [&](VkInstance _instance, GLFWwindow* _windows)->VkResult {
		return glfwCreateWindowSurface(_instance, _windows, nullptr, &m_surface);
		};
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
			if ( (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !queueIndices.isCompleteCompute())
				queueIndices.computeFamily = i;
			if (queueIndices.isCompleteGraphic() && queueIndices.isCompleteGraphic())
				break;
			++i;
		}
		if (surface && queueIndices.isCompleteGraphic()) {
			for (const auto& queueFamily : queueFamilies) {
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
				if (presentSupport && i != queueIndices.graphicsFamily.value()) {
					queueIndices.presentFamily = i;
					break;
				}
				++i;
			}
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
		if (m_bGlfw && extensionsSupported && m_surface) {
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
			&& indices.isCompleteGraphic() && indices.isCompleteCompute() &&
			( !m_bGlfw || ( m_bGlfw && extensionsSupported && swapChainAdequate && indices.isCompletePresent()) );

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
	if (m_bGlfw) {
		m_enableDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		VkResult ret = createSurface(m_instance, m_windows);
		if (ret != VK_SUCCESS) {
			std::cout << "create surface failed!" << std::endl;
			return VK_ERROR_INITIALIZATION_FAILED;
		}
	}
	uint32_t deviceCnt = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCnt, nullptr);
	if (deviceCnt == 0) {
		std::cout << "failed to find GPUs" << std::endl;
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	std::vector<VkPhysicalDevice> devices(deviceCnt);
	vkEnumeratePhysicalDevices(m_instance, &deviceCnt, devices.data());
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
VkResult VKInstance::createDeviceAndQueue()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	//std::set<uint32_t> uniqueQueueFailies = { m_queueIndices.graphicsFamily.value(), m_queueIndices.presentFamily.value(), m_queueIndices.computeFamily.value() };
	std::set<uint32_t> uniqueQueueFailies;
	if (m_queueIndices.graphicsFamily.has_value())
		uniqueQueueFailies.insert(m_queueIndices.graphicsFamily.value());
	if (m_queueIndices.presentFamily.has_value())
		uniqueQueueFailies.insert(m_queueIndices.presentFamily.value());
	if (m_queueIndices.computeFamily.has_value())
		uniqueQueueFailies.insert(m_queueIndices.computeFamily.value());
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
	if (m_queueIndices.isCompletePresent()) {
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
VkResult VKInstance::createCommandPool()
{
	VkCommandPoolCreateInfo cmdPoolCreateInfo{};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkResult ret = VK_SUCCESS;
	if (m_queueIndices.isCompleteGraphic()) {
		cmdPoolCreateInfo.queueFamilyIndex = m_queueIndices.graphicsFamily.value();
		ret = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_graphicsCommandPool);
		if (ret != VK_SUCCESS)
			return ret;
	}
	if (m_queueIndices.isCompletePresent()) {
		uint32_t fi = m_queueIndices.presentFamily.value();
		if (fi == m_queueIndices.graphicsFamily.value()) {
			if (ret == VK_SUCCESS)
				m_presentCommandPool = m_graphicsCommandPool;
			else
				return ret;
		}
		else {
			cmdPoolCreateInfo.queueFamilyIndex = m_queueIndices.presentFamily.value();
			ret = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_presentCommandPool);
			if (ret != VK_SUCCESS)
				return ret;
		}
	}
	if (m_queueIndices.isCompleteCompute()) {
		uint32_t fi = m_queueIndices.computeFamily.value();
		if (fi == m_queueIndices.graphicsFamily.value()) {
			if (ret == VK_SUCCESS)
				m_computeCommandPool = m_graphicsCommandPool;
			else
				return ret;
		}
		else if (m_queueIndices.presentFamily.value() == fi) {
			if (ret == VK_SUCCESS)
				m_computeCommandPool = m_presentCommandPool;
			else
				return ret;
		}
		else {
			cmdPoolCreateInfo.queueFamilyIndex = m_queueIndices.computeFamily.value();
			ret = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_computeCommandPool);
		}
	}
	return ret;
}




Abr::Abr()
{
}
Abr::~Abr()
{
}



VKFence::VKFence()
{
	m_device = GetLYJVKInstance()->m_device;
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_fence);
}
VKFence::~VKFence()
{
	if(m_fence)
		vkDestroyFence(m_device, m_fence, nullptr);
}
inline VkFence VKFence::ptr() { return m_fence; };
inline void VKFence::wait() { vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX); };
inline void VKFence::reset() { vkResetFences(m_device, 1, &m_fence); };




VKSemaphore::VKSemaphore()
{
	m_device = GetLYJVKInstance()->m_device;
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphore);
}
VKSemaphore::~VKSemaphore()
{
	if(m_semaphore)
		vkDestroySemaphore(m_device, m_semaphore, nullptr);
}
inline VkSemaphore VKSemaphore::ptr() { return m_semaphore; }



VULKAN_LYJ_API VKInstance* GetLYJVKInstance()
{
	return VKInstance::GetVKInstance();
}

NSP_VULKAN_LYJ_END


