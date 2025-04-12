#ifndef VULKAN_TEST_H
#define VULKAN_TEST_H
//#include <vulkan/vulkan.hpp>
//#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <fstream>

#include <opencv2/opencv.hpp>

#include "../VulkanCommon.h"
#include "../VulkanBuffer.h"
#include "../VulkanPipeline.h"
#include "../VulkanSwapChain.h"
#include "../VulkanImplement.h"


class VULKAN_LYJ_API VKComputeTest
{
public:
	VKComputeTest();
	~VKComputeTest();

	bool init();
	void run();
	void cleanup();

private:
	std::shared_ptr<LYJ_VK::VKBufferUniform> m_uniBuffer = nullptr;
	std::shared_ptr<LYJ_VK::VKBufferUniform> m_uniBuffer2 = nullptr;
	std::shared_ptr<LYJ_VK::VKBufferCompute> m_devBuffer = nullptr;
	std::shared_ptr<LYJ_VK::VKBufferCompute> m_devBuffer2 = nullptr;

	std::shared_ptr<LYJ_VK::VKPipelineCompute> m_com1 = nullptr;
	std::shared_ptr<LYJ_VK::VKPipelineCompute> m_com2 = nullptr;
	std::shared_ptr<LYJ_VK::VKImp> m_imp = nullptr;
};



struct ShaderData
{
	glm::mat4 moduleMatrix; //局部坐标系转换到世界坐标系
	glm::mat4 viewMatrix; //世界坐标系转换到相机坐标系
	glm::mat4 projectionMatrix; //相机坐标系转换到裁剪坐标系
};
struct Vertex
{
	float pos[3];
	float uv[2];
	float normal[3];
};
class VULKAN_LYJ_API VKGraphicTest
{
public:
	VKGraphicTest();
	~VKGraphicTest();

	void run();

private:
	void init();
	void mainLoop();
	void drawFrame();
	void cleanup();

private:
	LYJ_VK::VKInstance* m_lyjVK;

	std::shared_ptr<LYJ_VK::VKSwapChain> m_swapChain = nullptr;
	std::shared_ptr<LYJ_VK::VKPipelineGraphics> m_pipelineGraphics = nullptr;
	std::shared_ptr<LYJ_VK::VKBufferImage> m_image = nullptr;
	std::vector<std::shared_ptr<LYJ_VK::VKBufferUniform>> m_uniBuffers;
	std::shared_ptr<LYJ_VK::VKBufferVertex> m_verBuffer = nullptr;
	std::shared_ptr<LYJ_VK::VKBufferIndex> m_indBuffer = nullptr;
	std::vector< std::shared_ptr<LYJ_VK::VKBufferImage>> m_imgs;
	std::shared_ptr<LYJ_VK::VKBufferImage> m_depthImage = nullptr;
	std::vector < std::shared_ptr<LYJ_VK::VKImp>> m_imps;
	int m_cnt = 0;

	std::shared_ptr < LYJ_VK::VKFence> m_fence = nullptr;
	std::shared_ptr < LYJ_VK::VKSemaphore> m_availableSemaphore = nullptr;
	std::shared_ptr < LYJ_VK::VKSemaphore> m_finishedSemaphore = nullptr;
	std::vector < std::shared_ptr<LYJ_VK::VKCommandImageBarrier>> m_cmdImgBarriers;

};




#endif //VULKAN_TEST_H

