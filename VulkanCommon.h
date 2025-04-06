#ifndef VULKAN_COMMON_H
#define VULKAN_COMMON_H

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <optional>
#include <iostream>
#include <set>


//export
#ifdef WIN32
#ifdef _MSC_VER
#define VULKAN_LYJ_API __declspec(dllexport)
#else
#define VULKAN_LYJ_API
#endif
#else
#define VULKAN_LYJ_API
#endif

#define NSP_VULKAN_LYJ_BEGIN namespace LYJ_VK {
#define NSP_VULKAN_LYJ_END }

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << "FAIL" << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

NSP_VULKAN_LYJ_BEGIN


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}


struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	bool isCompleteGraphic() {
		return graphicsFamily.has_value();
	}
	std::optional<uint32_t> presentFamily;
	bool isCompletePresent() {
		return presentFamily.has_value();
	}
	std::optional<uint32_t> computeFamily;
	bool isCompleteCompute() {
		return computeFamily.has_value();
	}
};
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};
class VULKAN_LYJ_API VKInstance
{
public:
	VKInstance();
	~VKInstance();

	VKInstance(const VKInstance&) = delete;
	VKInstance& operator=(const VKInstance&) = delete;

	//µ¥Àý
	static VKInstance* GetVKInstance()
	{
		static VKInstance instance;
		return &instance;
	}
	//inline void pollEvents() { glfwPollEvents(); };

	bool isInited();
	VkResult init(bool _bGlfw=false, GLFWwindow* _windows=nullptr, bool _bValid=false);
	void clean();

	VkQueue getGraphicQueue(int _i);
	VkQueue getPresentQueue(int _i);
	VkQueue getComputeQueue(int _i);

	uint32_t getMemoryTypeIndex(uint32_t _typeBits, VkMemoryPropertyFlags _properties);

private:
	VkResult createInstance();
	VkResult createPhysicalDevice();
	VkResult createDeviceAndQueue();
	VkResult createCommandPool();


	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(instance, debugMessenger, pAllocator);
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}
//
public:
	bool m_init = false;
	VkInstance m_instance = VK_NULL_HANDLE;

	bool m_bGlfw = false;
	std::vector<const char*> m_supportInstanceExtensions;
	std::vector<const char*> m_enableInstanceExtensions;
	bool m_bValid = false;
	std::vector<const char*> m_supportLayers;
	std::vector<const char*> m_enableLayers;
	VkDebugUtilsMessengerEXT m_debugMessenger;

	GLFWwindow* m_windows = nullptr;
	const uint32_t m_width = 1600;
	const uint32_t m_height = 1200;
	VkSurfaceKHR m_surface;
	std::vector<VkQueueFamilyProperties> m_queueFamilies;
	QueueFamilyIndices m_queueIndices;
	std::vector<const char*> m_supportDeviceExtensions;
	std::vector<const char*> m_enableDeviceExtensions;
	SwapChainSupportDetails m_details;
	VkPhysicalDeviceMemoryProperties m_memProperties{};
	VkPhysicalDeviceFeatures m_devFeature{};
	VkPhysicalDeviceProperties m_devProperties{};
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	VkDevice m_device = VK_NULL_HANDLE;
	std::vector<VkQueue> m_graphicQueues;
	std::vector<VkQueue> m_presentQueues;
	std::vector<VkQueue> m_computeQueues;

	VkCommandPool m_graphicsCommandPool;
	VkCommandPool m_presentCommandPool;
	VkCommandPool m_computeCommandPool;
};



class Abr
{
public:
	Abr();
	~Abr();
	virtual void destroy() = 0;
private:

};

enum class VULKAN_LYJ_API BASETYPE
{
	UINT8=0,
	INT8,
	UINT16,
	INT16,
	UINT32,
	INT32,
	FLOAT32,
	UINT64, //no use
	INT64,
	FLOAT64
};




/// <summary>
/// synchronous cpu and current queue
/// </summary>
class VULKAN_LYJ_API VKFence
{
public:
	VKFence();
	~VKFence();
	VkFence ptr();
	void wait();
	void reset();
private:
	VkFence m_fence = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
};




/// <summary>
/// synchronous deference queue
/// </summary>
class VULKAN_LYJ_API VKSemaphore
{
public:
	VKSemaphore();
	~VKSemaphore();
	VkSemaphore ptr();
private:
	VkSemaphore m_semaphore = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
};


VULKAN_LYJ_API VKInstance* GetLYJVKInstance();


NSP_VULKAN_LYJ_END





#endif // !VULKAN_COMMON_H
