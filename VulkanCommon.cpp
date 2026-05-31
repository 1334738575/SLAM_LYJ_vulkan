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
		if (_windows == nullptr)
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
	for (auto pool : m_commandPools) {
		if (pool)
			vkDestroyCommandPool(m_device, pool, nullptr);
	}
	m_commandPools.clear();
	m_commandPoolQueues.clear();
	m_graphicsCommandPool = VK_NULL_HANDLE;
	m_presentCommandPool = VK_NULL_HANDLE;
	m_computeCommandPool = VK_NULL_HANDLE;

	vkDestroyDevice(m_device, nullptr);
	if (m_bGlfw) {
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	}
	if (m_bValid)
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
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
VkCommandPool VKInstance::getCommandPool(VkQueue queue)
{
	for (size_t i = 0; i < m_commandPoolQueues.size(); ++i) {
		if (m_commandPoolQueues[i] == queue)
			return m_commandPools[i];
	}
	return m_graphicsCommandPool;
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
			if (std::find_if(m_supportInstanceExtensions.begin(), m_supportInstanceExtensions.end(), [&](const char* _v1) {
				return strcmp(_v1, enabledExtension) == 0;
				}
			) == m_supportInstanceExtensions.end())
				//if (std::find(m_supportInstanceExtensions.begin(), m_supportInstanceExtensions.end(), enabledExtension) == m_supportInstanceExtensions.end())
				std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
			m_enableInstanceExtensions.push_back(enabledExtension);
		}
	}
	//if (m_bValid || std::find(m_supportInstanceExtensions.begin(), m_supportInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != m_supportInstanceExtensions.end()) {
	if (m_bValid ||
		std::find_if(m_supportInstanceExtensions.begin(), m_supportInstanceExtensions.end(), [&](const char* _v) {
			return strcmp(_v, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
			}) != m_supportInstanceExtensions.end()) {
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
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}
	VkResult ret = vkCreateInstance(&createInfo, nullptr, &m_instance);
	if (m_bValid) {
		VkDebugUtilsMessengerCreateInfoEXT createMessageInfo;
		populateDebugMessengerCreateInfo(createMessageInfo);
		ret = CreateDebugUtilsMessengerEXT(m_instance, &createMessageInfo, nullptr, &m_debugMessenger);
	}
	return ret;
}
VkResult VKInstance::createPhysicalDevice()
{
	auto createSurface = [&](VkInstance _instance, GLFWwindow* _windows)->VkResult {
		return glfwCreateWindowSurface(_instance, _windows, nullptr, &m_surface);
		};
	auto funcFindQueueFamilies = [](VkPhysicalDevice device, std::vector<VkQueueFamilyProperties>& queueFamilies, VkSurfaceKHR surface = nullptr) -> QueueFamilyIndices {
		QueueFamilyIndices queueIndices;
		uint32_t queueFamilyCnt = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCnt, nullptr);
		queueFamilies.resize(queueFamilyCnt);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCnt, queueFamilies.data());
		bool bFind = false;
		int i = 0;
		VkBool32 presentSupport = false;
		for (const auto& queueFamily : queueFamilies) {
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !queueIndices.isCompleteGraphic())
				queueIndices.graphicsFamily = i;
			if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !queueIndices.isCompleteCompute())
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

		bool ret = devFeature.geometryShader\
			&& indices.isCompleteGraphic() && indices.isCompleteCompute() &&
			(!m_bGlfw || (m_bGlfw && extensionsSupported && swapChainAdequate && indices.isCompletePresent()));

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
	//m_enableDeviceExtensions.push_back("VK_KHR_separate_depth_stencil_layouts");
	//m_enableDeviceExtensions.push_back("VK_KHR_get_physical_device_properties2");
	//m_enableDeviceExtensions.push_back("VK_KHR_create_renderpass2");
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
	int bestScore = -1;
	for (const auto& device : devices) {
		if (funcDeviceSuitable(device)) {
			int score = 0;
			if (m_devProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				score += 1000;
			else if (m_devProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				score += 500;
			score += static_cast<int>(m_devProperties.limits.maxImageDimension2D);
			if (score > bestScore) {
				bestScore = score;
				m_physicalDevice = device;
			}
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
	std::vector<std::vector<float>> queuePros(3);
	int cnt = 0;
	//std::vector<float> queuePros2;
	//std::vector<float> queuePros3;
	for (uint32_t queueFamily : uniqueQueueFailies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = m_queueFamilies[queueFamily].queueCount;
		queuePros[cnt].assign(m_queueFamilies[queueFamily].queueCount, queuePriority);
		queueCreateInfo.pQueuePriorities = &queuePros[cnt].front();
		queueCreateInfos.push_back(queueCreateInfo);
		++cnt;
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
		else if (m_queueIndices.isCompletePresent() && m_queueIndices.presentFamily.value() == fi) {
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

	auto createPoolsForQueues = [&](uint32_t queueFamilyIndex, const std::vector<VkQueue>& queues, VkCommandPool& firstPool)->VkResult {
		firstPool = VK_NULL_HANDLE;
		for (size_t i = 0; i < queues.size(); ++i) {
			cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
			VkCommandPool pool = VK_NULL_HANDLE;
			VkResult ret = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &pool);
			if (ret != VK_SUCCESS)
				return ret;
			m_commandPoolQueues.push_back(queues[i]);
			m_commandPools.push_back(pool);
			if (i == 0)
				firstPool = pool;
		}
		return VK_SUCCESS;
		};

	VkResult ret = VK_SUCCESS;
	if (m_queueIndices.isCompleteGraphic()) {
		ret = createPoolsForQueues(m_queueIndices.graphicsFamily.value(), m_graphicQueues, m_graphicsCommandPool);
		if (ret != VK_SUCCESS)
			return ret;
	}
	if (m_queueIndices.isCompletePresent()) {
		uint32_t fi = m_queueIndices.presentFamily.value();
		if (m_queueIndices.isCompleteGraphic() && fi == m_queueIndices.graphicsFamily.value()) {
			m_presentCommandPool = m_graphicsCommandPool;
		}
		else {
			ret = createPoolsForQueues(fi, m_presentQueues, m_presentCommandPool);
			if (ret != VK_SUCCESS)
				return ret;
		}
	}
	if (m_queueIndices.isCompleteCompute()) {
		uint32_t fi = m_queueIndices.computeFamily.value();
		if (m_queueIndices.isCompleteGraphic() && fi == m_queueIndices.graphicsFamily.value()) {
			m_computeCommandPool = m_graphicsCommandPool;
		}
		else if (m_queueIndices.isCompletePresent() && fi == m_queueIndices.presentFamily.value()) {
			m_computeCommandPool = m_presentCommandPool;
		}
		else {
			ret = createPoolsForQueues(fi, m_computeQueues, m_computeCommandPool);
			if (ret != VK_SUCCESS)
				return ret;
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
	if (m_fence)
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
	if (m_semaphore)
		vkDestroySemaphore(m_device, m_semaphore, nullptr);
}
inline VkSemaphore VKSemaphore::ptr() { return m_semaphore; }



VULKAN_LYJ_API VKInstance* GetLYJVKInstance()
{
	return VKInstance::GetVKInstance();
}

NSP_VULKAN_LYJ_END


