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
