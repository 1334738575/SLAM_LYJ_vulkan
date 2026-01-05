#include <common/BaseTriMesh.h>
#include <IO/MeshIO.h>
#include <base/Pose.h>
#include <base/CameraModule.h>
#include <IO/SimpleIO.h>
#include <STLPlus/include/file_system.h>


#include "VulkanTest.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <vulkan/vulkan.hpp>
#include <projector/ProjectorVK.h>

#include <Eigen/Eigen>

//static std::string shaderPath = "D:/testLyj/Vulkan/shader/";
static std::string vulkanHomePath(VULKAN_LYJ_HOME_PATH);
static std::string shaderPath = vulkanHomePath + "/shader/";
static std::string imagePath = "D:/SLAM_LYJ/other/";


VKComputeTest::VKComputeTest()
{
	init();
}
VKComputeTest::~VKComputeTest()
{
	cleanup();
}
bool VKComputeTest::init()
{
	LYJ_VK::VKInstance *lyjVK = LYJ_VK::GetLYJVKInstance();
	if (!lyjVK->isInited())
	{
		if (lyjVK->init(false, nullptr, true) != VK_SUCCESS)
		{
			std::cout << "init vulkan fail!" << std::endl;
			return false;
		}
	}
	return true;
}
void VKComputeTest::run()
{
	LYJ_VK::VKInstance *lyjVK = LYJ_VK::GetLYJVKInstance();
	VkDevice device = lyjVK->m_device;
	VkQueue computeQueue = lyjVK->m_computeQueues[0];
	VkCommandPool computeCommandPool = lyjVK->m_computeCommandPool;

	// generate data
	const int computeSize = 32;
	const VkDeviceSize bufferSize = computeSize * sizeof(uint32_t);
	uint32_t n = 0;
	std::vector<uint32_t> computeInput(computeSize);
	std::vector<uint32_t> computeOutput(computeSize);
	std::generate(computeInput.begin(), computeInput.end(), [&n]
				  { return n++; });
	std::vector<uint32_t> computeInput2(computeSize);
	std::vector<uint32_t> computeOutput2(computeSize);
	uint32_t n2 = 5;
	std::generate(computeInput2.begin(), computeInput2.end(), [&n2]
				  { return n2++; });
	const VkDeviceSize bufferSize2 = computeSize * sizeof(uint32_t);

	// upload
	m_uniBuffer.reset(new LYJ_VK::VKBufferUniform());
	uint32_t dataSize = 32;
	m_uniBuffer->upload(sizeof(uint32_t), &dataSize, computeQueue);
	m_devBuffer.reset(new LYJ_VK::VKBufferCompute());
	m_devBuffer->upload(bufferSize, computeInput.data(), computeQueue);
	vkQueueWaitIdle(computeQueue);
	m_devBuffer2.reset(new LYJ_VK::VKBufferCompute());
	m_devBuffer2->upload(bufferSize2, computeInput2.data(), computeQueue);
	m_uniBuffer2.reset(new LYJ_VK::VKBufferUniform());
	uint32_t dataSize2 = 16;
	m_uniBuffer2->upload(sizeof(uint32_t), &dataSize2, computeQueue);

	// pipeline
	m_com1.reset(new LYJ_VK::VKPipelineCompute("D:/testLyj/Vulkan/shader/compute/headless2.comp.spv"));
	m_com1->setBufferBinding(0, m_uniBuffer.get());
	m_com1->setBufferBinding(1, m_devBuffer.get());
	m_com1->setBufferBinding(2, m_devBuffer2.get());
	m_com1->setRunKernel(32);
	VK_CHECK_RESULT(m_com1->build());
	m_com2.reset(new LYJ_VK::VKPipelineCompute("D:/testLyj/Vulkan/shader/compute/headless3.comp.spv"));
	m_com2->setBufferBinding(0, m_uniBuffer2.get());
	m_com2->setBufferBinding(1, m_devBuffer.get());
	m_com2->setBufferBinding(2, m_devBuffer2.get());
	m_com2->setRunKernel(32);
	VK_CHECK_RESULT(m_com2->build());
	LYJ_VK::VKCommandBufferBarrier cmdBufferBarrier(
		{m_devBuffer->getBuffer(), m_devBuffer2->getBuffer()},
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	LYJ_VK::VKCommandBufferBarrier cmdBufferBarrier2(
		{m_devBuffer->getBuffer(), m_devBuffer2->getBuffer()},
		VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	LYJ_VK::VKCommandMemoryBarrier endBarrier;
	m_imp.reset(new LYJ_VK::VKImp(0));
	m_imp->setCmds({&cmdBufferBarrier, m_com1.get(), &cmdBufferBarrier2, m_com2.get(), &endBarrier});

	// compute
	LYJ_VK::VKFence fence;
	m_imp->run(computeQueue, fence.ptr());
	fence.wait();
	fence.reset();

	// release copy buffer
	m_devBuffer2->releaseBufferCopy();
	m_devBuffer->releaseBufferCopy();

	// download
	// uint32_t* retPtr2 = (uint32_t*)m_devBuffer2->download(bufferSize, computeOutput2.data(), computeQueue);
	// uint32_t* retPtr = (uint32_t*)m_devBuffer->download(bufferSize, computeOutput.data(), computeQueue);
	uint32_t *retPtr2 = (uint32_t *)m_devBuffer2->download(bufferSize, computeQueue);
	uint32_t *retPtr = (uint32_t *)m_devBuffer->download(bufferSize, computeQueue, fence.ptr());
	fence.wait();
	memcpy(computeOutput2.data(), retPtr2, bufferSize);
	memcpy(computeOutput.data(), retPtr, bufferSize);
	m_devBuffer2->releaseBufferCopy();
	m_devBuffer->releaseBufferCopy();

	// Output
	std::cout << "Compute input:\n";
	for (auto v : computeInput)
	{
		std::cout << v << " ";
	}
	std::cout << std::endl;
	std::cout << "Compute output:\n";
	for (auto v : computeOutput)
	{
		std::cout << v << " ";
	}
	std::cout << std::endl;
	for (auto v : computeOutput2)
	{
		std::cout << v << " ";
	}
	std::cout << std::endl;
}
void VKComputeTest::cleanup()
{
	if (m_devBuffer)
		m_devBuffer->destroy();
	if (m_uniBuffer)
		m_uniBuffer->destroy();
	if (m_devBuffer2)
		m_devBuffer2->destroy();
	if (m_uniBuffer2)
		m_uniBuffer2->destroy();
	if (m_imp)
		m_imp->destroy();
	if (m_com1)
		m_com1->destroy();
	if (m_com2)
		m_com2->destroy();
}

VKGraphicTest::VKGraphicTest()
{
	m_bPresent = false;
	if (!m_bPresent)
		m_imgSize = 1;
	if (m_bPresent)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	}
	m_lyjVK = LYJ_VK::GetLYJVKInstance();
	if (!m_lyjVK->isInited())
	{
		if (m_lyjVK->init(m_bPresent, nullptr, true) != VK_SUCCESS)
		{
			std::cout << "init vulkan fail!" << std::endl;
			return;
		}
	}
}
VKGraphicTest::~VKGraphicTest()
{
	cleanup();
}
void VKGraphicTest::run()
{
	init();
	if (m_bPresent)
		mainLoop();
	else
		drawFrame();
}
void VKGraphicTest::init()
{
	m_pipelineGraphics.reset(new LYJ_VK::VKPipelineGraphics(shaderPath + "texture/texture.vert.spv", shaderPath + "texture/texture.frag.spv", m_imgSize));
	if (m_bPresent)
		m_swapChain.reset(new LYJ_VK::VKSwapChain(m_imgSize));

	// texture2d
	auto funcCreateTextureImage = [&]()
	{
		VkDevice device = m_lyjVK->m_device;
		VkCommandPool graphicsCommandPool = m_lyjVK->m_graphicsCommandPool;
		VkQueue graphicQueue = m_lyjVK->m_graphicQueues[0];
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties = m_lyjVK->m_memProperties;
		uint32_t imgCnt = m_imgSize;

		std::string imgName = imagePath + "IMG_9179[1](1).png";
		cv::Mat image1 = cv::imread(imgName, 0);
		cv::Mat image2 = image1.clone();
		if (image1.empty() || image2.empty())
		{
			throw std::runtime_error("failed to load texture image!");
		}
		cv::Mat image = cv::Mat::zeros(image1.rows, image1.cols + image2.cols, image1.type());
		cv::Rect rect1(0, 0, image1.cols, image1.rows);
		cv::Rect rect2(image1.cols, 0, image2.cols, image2.rows);
		image1.copyTo(image(rect1));
		image2.copyTo(image(rect2));
		if (image.channels() == 1)
			cv::cvtColor(image, image, cv::COLOR_GRAY2RGBA);

		const int width = image.cols;
		const int height = image.rows;
		m_image.reset(new LYJ_VK::VKBufferSamplerImage(width, height, 4, 1, LYJ_VK::VKBufferImage::IMAGEVALUETYPE::UINT8));
		// m_image->upload(width* height * 4, image.data, graphicQueue);
		LYJ_VK::VKFence fence;
		m_image->upload(width * height * 4, image.data, graphicQueue, fence.ptr());
		fence.wait();
		m_image->releaseBufferCopy();
		for (int i = 0; i < imgCnt; ++i)
			m_pipelineGraphics->setBufferBinding(3, m_image.get(), i);
	};
	// funcCreateTextureImage();
	// return;

	// vertices and indices buffer
	const float edgeRatio = 0.5;
	const float textureEdgeRatio = 1.0;
	auto funcGenerateQuad = [&]()
	{
		VkQueue graphicQueue = m_lyjVK->m_graphicQueues[0];
		std::vector<Vertex> vertices =
			{
				{{edgeRatio, edgeRatio, 0.5f}, {textureEdgeRatio, textureEdgeRatio}, {0.f, 0.f, 1.f}},
				{{-edgeRatio, edgeRatio, 0.5f}, {0.f, textureEdgeRatio}, {0.f, 0.f, 1.f}},
				{{-edgeRatio, -edgeRatio, 0.5f}, {0.f, 0.f}, {0.f, 0.f, 1.f}},
				{{edgeRatio, -edgeRatio, 0.5f}, {textureEdgeRatio, 0.f}, {0.f, 0.f, 1.f}}};
		std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
		m_verBuffer.reset(new LYJ_VK::VKBufferVertex());
		m_verBuffer->upload(vertices.size() * sizeof(Vertex), vertices.data(), graphicQueue);
		m_indBuffer.reset(new LYJ_VK::VKBufferIndex());
		m_indBuffer->upload(indices.size() * sizeof(uint32_t), indices.data(), graphicQueue);
		LYJ_VK::ClassResolver classResolver;
		classResolver.addBindingDescriptor(0, sizeof(Vertex));
		classResolver.addAttributeDescriptor(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos));
		classResolver.addAttributeDescriptor(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
		classResolver.addAttributeDescriptor(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));
		m_pipelineGraphics->setVertexBuffer(m_verBuffer.get(), vertices.size(), classResolver);
		m_pipelineGraphics->setIndexBuffer(m_indBuffer.get(), indices.size());
		vkQueueWaitIdle(m_lyjVK->m_graphicQueues[0]);
		m_verBuffer->releaseBufferCopy();
		m_indBuffer->releaseBufferCopy();
	};
	funcGenerateQuad();
	// return;

	// uniform buffer
	auto funcCreateUniformBuffers = [&]()
	{
		uint32_t imageCnt = m_imgSize;
		m_uniBuffers.resize(imageCnt, nullptr);
		for (uint32_t i = 0; i < m_uniBuffers.size(); ++i)
		{
			m_uniBuffers[i].reset(new LYJ_VK::VKBufferUniform());
			m_uniBuffers[i]->resize(sizeof(ShaderData));
			m_pipelineGraphics->setBufferBinding(0, m_uniBuffers[i].get(), i);
		}
	};
	funcCreateUniformBuffers();
	// return;

	// pipeline
	{
		if (m_bPresent)
		{
			std::vector<std::shared_ptr<LYJ_VK::VKBufferImage>> &images = m_swapChain->getImages();
			for (size_t i = 0; i < m_swapChain->getImageCnt(); ++i)
			{
				m_pipelineGraphics->setImage(i, 0, images[i]);
			}
		}
		int h = m_lyjVK->m_height;
		int w = m_lyjVK->m_width;
		for (size_t i = 0; i < m_imgSize; ++i)
		{
			std::shared_ptr<LYJ_VK::VKBufferImage> img;
			std::shared_ptr<LYJ_VK::VKBufferImage> img2;
			std::shared_ptr<LYJ_VK::VKBufferImage> img3;
			img.reset(new LYJ_VK::VKBufferColorImage(w, h, 4, 1, LYJ_VK::VKBufferImage::IMAGEVALUETYPE::UINT8));
			img2.reset(new LYJ_VK::VKBufferColorImage(w, h, 1, 4, LYJ_VK::VKBufferImage::IMAGEVALUETYPE::UINT32));
			img3.reset(new LYJ_VK::VKBufferColorImage(w, h, 4, 1, LYJ_VK::VKBufferImage::IMAGEVALUETYPE::UINT8));
			m_pipelineGraphics->setImage(i, 1 - int(!m_bPresent), img);
			m_pipelineGraphics->setImage(i, 2 - int(!m_bPresent), img2);
			m_pipelineGraphics->setImage(i, 3 - int(!m_bPresent), img3);
			m_imgs.push_back(img);
			m_imgs.push_back(img2);
			m_imgs.push_back(img3);
		}
		m_depthImage.reset(new LYJ_VK::VKBufferDepthImage(w, h));
		m_pipelineGraphics->setDepthImage(m_depthImage);
	}
	m_pipelineGraphics->build();
	// return;

	// imp
	m_imps.resize(m_imgSize, nullptr);
	for (int i = 0; i < m_imgSize; ++i)
	{
		m_imps[i].reset(new LYJ_VK::VKImp(0));
		m_imps[i]->setCmds({m_pipelineGraphics.get()});
	}
	// return;

	// fence and semaphore
	m_fence.reset(new LYJ_VK::VKFence());
	m_availableSemaphore.reset(new LYJ_VK::VKSemaphore());
	m_finishedSemaphore.reset(new LYJ_VK::VKSemaphore());
	m_cmdImgBarriers.resize(m_imgSize, nullptr);
	for (int i = 0; i < m_imgSize; ++i)
	{
		if (m_bPresent)
			m_cmdImgBarriers[i].reset(new LYJ_VK::VKCommandImageBarrier(
				{m_pipelineGraphics->getImage(i, 0)->getImage()}, {m_pipelineGraphics->getImage(i, 0)->getSubresource()},
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED));
		else
			m_cmdImgBarriers[i].reset(new LYJ_VK::VKCommandImageBarrier(
				{m_pipelineGraphics->getImage(i, 0)->getImage()}, {m_pipelineGraphics->getImage(i, 0)->getSubresource()},
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED));
	}
	return;
}
void VKGraphicTest::mainLoop()
{
	VkDevice device = m_lyjVK->m_device;
	GLFWwindow *windows = m_lyjVK->m_windows;

	while (!glfwWindowShouldClose(windows))
	{
		drawFrame();
		glfwPollEvents();
		m_fence->wait();
		m_fence->reset();
	}
	vkDeviceWaitIdle(device);
	glfwTerminate();
}
void VKGraphicTest::drawFrame()
{
	VkDevice device = m_lyjVK->m_device;
	VkQueue graphicQueue = m_lyjVK->m_graphicQueues[0];
	VkQueue presentQueue = nullptr;
	VkSwapchainKHR swapChain = nullptr;
	uint32_t imageIndex = 0;

	if (m_bPresent)
	{
		presentQueue = m_lyjVK->m_presentQueues[0];
		swapChain = m_swapChain->getSwapChain();
		vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, m_availableSemaphore->ptr(), VK_NULL_HANDLE, &imageIndex);
		m_pipelineGraphics->setCurId(imageIndex);
	}
	ShaderData shaderData{};
	float xof = (m_cnt++ / 100) * 0.1f;
	// xof = 0.0f;
	shaderData.projectionMatrix = glm::mat4( // 列为主
		1.0f, 0.0f, 0.0f, 0.0f,				 // 第一列
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		xof, 0.0f, 0.0f, 1.0f);
	const VkExtent2D &extent2D = m_swapChain->getExtent2D();
	// shaderData.projectionMatrix = glm::perspective(glm::radians(-90.0f), (float)extent2D.width / (float)extent2D.width, 0.1f, 2.f);
	// shaderData.projectionMatrix *= -1;
	shaderData.viewMatrix = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	// shaderData.moduleMatrix = glm::mat4(1.0f);
	shaderData.moduleMatrix = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	m_uniBuffers[imageIndex]->upload(sizeof(ShaderData), &shaderData, graphicQueue);
	if (m_bPresent)
	{
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		m_imps[imageIndex]->run(graphicQueue, m_fence->ptr(), {m_availableSemaphore->ptr()}, {m_finishedSemaphore->ptr()}, waitStages);
	}
	else
		m_imps[imageIndex]->run(graphicQueue, m_fence->ptr());

	LYJ_VK::VKImp impTmp(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	impTmp.setCmds({m_cmdImgBarriers[imageIndex].get()});
	LYJ_VK::VKFence fenceTmp;
	impTmp.run(graphicQueue, fenceTmp.ptr());
	fenceTmp.wait();
	impTmp.destroy();
	// get image in swapchain
	if (m_cnt && true)
	{
		int h = m_lyjVK->m_height;
		int w = m_lyjVK->m_width;

		{
			auto lyjImg = m_pipelineGraphics->getImage(0, 1 - int(!m_bPresent));
			int c = lyjImg->getChannels();
			int step = lyjImg->getStep();
			int s = w * h * c * step;
			LYJ_VK::VKFence fenceTmp;
			void *data = lyjImg->download(s, graphicQueue, fenceTmp.ptr());
			fenceTmp.wait();
			cv::Mat mmm(h, w, CV_8UC4);
			// cv::Mat mmm(h, w, CV_32FC4);
			memcpy(mmm.data, data, s);
			lyjImg->releaseBufferCopy();
			cv::imshow("111", mmm);
			// cv::waitKey();
		}
		{
			auto lyjImg = m_pipelineGraphics->getImage(0, 2 - int(!m_bPresent));
			int c = lyjImg->getChannels();
			int step = lyjImg->getStep();
			int s = w * h * c * step;
			LYJ_VK::VKFence fenceTmp;
			void *data = lyjImg->download(s, graphicQueue, fenceTmp.ptr());
			fenceTmp.wait();
			cv::Mat mmm(h, w, CV_32SC1);
			cv::Mat mmm2(h, w, CV_8UC1);
			memcpy(mmm.data, data, s);
			lyjImg->releaseBufferCopy();
			// mmm *= 127;
			for (int i = 0; i < h; ++i)
			{
				for (int j = 0; j < w; ++j)
				{
					int fid = mmm.at<int>(i, j);
					mmm2.at<uchar>(i, j) = (uchar)(fid * 127);
				}
			}
			cv::imshow("333", mmm2);
			// cv::waitKey();
		}
		{
			auto lyjDepth = m_pipelineGraphics->getDepthImage();
			int c = lyjDepth->getChannels();
			int step = lyjDepth->getStep();
			int s = w * h * c * step;
			LYJ_VK::VKFence fenceTmp;
			void *data = lyjDepth->download(s, graphicQueue, fenceTmp.ptr());
			fenceTmp.wait();
			cv::Mat mmm(h, w, CV_32FC1);
			memcpy(mmm.data, data, s);
			float *ppp = (float *)mmm.data;
			lyjDepth->releaseBufferCopy();
			cv::imshow("222", mmm);
			cv::waitKey();
		}
	}

	if (m_bPresent)
	{
		std::vector<VkSwapchainKHR> swapChains = {swapChain};
		std::vector<uint32_t> imageIndices = {imageIndex};
		std::vector<VkSemaphore> renderFinishedSemaphores = {m_finishedSemaphore->ptr()};
		LYJ_VK::VKImpPresent presentImp;
		if (presentImp.present(presentQueue, swapChains, imageIndices, renderFinishedSemaphores) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present image");
		}
	}
}
void VKGraphicTest::cleanup()
{
	m_fence = nullptr;
	m_availableSemaphore = nullptr;
	m_finishedSemaphore = nullptr;

	if (m_pipelineGraphics)
		m_pipelineGraphics->destroy();

	if (m_image)
		m_image->destroy();
	for (auto img : m_imgs)
		if (img)
			img->destroy();
	if (m_depthImage)
		m_depthImage->destroy();
	for (auto uniBuffer : m_uniBuffers)
		if (uniBuffer)
			uniBuffer->destroy();
	if (m_verBuffer)
		m_verBuffer->destroy();
	if (m_indBuffer)
		m_indBuffer->destroy();

	for (auto imp : m_imps)
		if (imp)
			imp->destroy();

	if (m_swapChain)
		m_swapChain->destory();

	m_lyjVK->clean();
}

void testVulkanGraphic()
{
	VKGraphicTest vulkan{};
	vulkan.run();
}
void testVulkanCompute()
{
	VKComputeTest vulkan{};
	vulkan.run();
}
void testProject()
{
	using namespace SLAM_LYJ;
	SLAM_LYJ::SLAM_LYJ_MATH::BaseTriMesh btm;
	SLAM_LYJ::readPLYMesh("D:/tmp/res_mesh.ply", btm);

	// data
	SLAM_LYJ::BaseTriMesh btmVulkan = btm;
	const auto& vertexs = btmVulkan.getVertexs();
	const auto& faces = btmVulkan.getFaces();
	btmVulkan.enableFCenters();
	btmVulkan.calculateFCenters();
	const auto& fCenters = btmVulkan.getFCenters();
	btmVulkan.enableFNormals();
	btmVulkan.calculateFNormals();
	const auto& fNormals = btmVulkan.getFNormals();
	int vn = btmVulkan.getVn();
	int fn = btmVulkan.getFn();
	int w = 2048;
	int h = 2048;
	std::vector<float> K{ 765.955, 766.549, 1024, 1024 };
	std::vector<double> Kd{ 765.955, 766.549, 1024, 1024 };
	Pose3D TcwP;
	TcwP.getR() << 0.00774473, 0.0383352, 0.999235,
		-0.0480375, 0.998125, -0.0379202,
		-0.998815, -0.047707, 0.00957183;
	TcwP.gett() << -0.0615093,
		-0.16702,
		0.011672;
	SLAM_LYJ::PinholeCamera cam(w, h, Kd);
	COMMON_LYJ::drawCam("D:/tmp/camVulkan.ply", cam, TcwP.inversed(), 10);
	Eigen::Matrix<float, 3, 4> T;
	T.block(0, 0, 3, 3) = TcwP.getR().cast<float>();
	T.block(0, 3, 3, 1) = TcwP.gett().cast<float>();


	LYJ_VK::ProjectorVK projectVK;
	projectVK.create(vertexs[0].data(), vn, fCenters[0].data(), fNormals[0].data(), faces[0].vId_, fn, K.data(), w, h);
	std::vector<uint> fIdsOut(w * h, UINT_MAX);
	std::vector<float> depthsOut(w * h, FLT_MAX);
	std::vector<char> PValidsOut(vn, 0);
	std::vector<char> fValidsOut(fn, 0);
	for (int i = 0; i < 100; ++i)
	{
		//projectVK.project(T.data(), depthsOut.data(), fIdsOut.data(), PValidsOut.data(), fValidsOut.data(), 0, 30, 0.0, 0.1);
		SLAM_LYJ::Pose3D Tcw2;
		std::string poseName = "D:/tmp/texture_data/RT_" + std::to_string(i) + ".txt";
		if (!stlplus::file_exists(poseName))
			continue;
		COMMON_LYJ::readT34(poseName, Tcw2);
		//COMMON_LYJ::drawCam("D:/tmp/camVulkan.ply", cam, Tcw2.inversed(), 10);
		Eigen::Matrix<float, 3, 4> T2;
		T2.block(0, 0, 3, 3) = Tcw2.getR().cast<float>();
		T2.block(0, 3, 3, 1) = Tcw2.gett().cast<float>();
		projectVK.project(T2.data(), depthsOut.data(), fIdsOut.data(), PValidsOut.data(), fValidsOut.data(), 0, 30, 0.0, 0.1);

		//{
		//	std::vector<Eigen::Vector3f> retPs;
		//	for (int i = 0; i < vn; ++i)
		//	{
		//		if (PValidsOut[i] == 0)
		//			continue;
		//		retPs.push_back(vertexs[i]);
		//	}
		//	SLAM_LYJ::BaseTriMesh btmTmp;
		//	btmTmp.setVertexs(retPs);
		//	SLAM_LYJ::writePLYMesh("D:/tmp/checkV.ply", btmTmp);
		//	std::vector<Eigen::Vector3f> retFs;
		//	for (int i = 0; i < fn; ++i)
		//	{
		//		if (fValidsOut[i] == 1)
		//			continue;
		//		retFs.push_back(fCenters[i]);
		//	}
		//	SLAM_LYJ::BaseTriMesh btmTmp2;
		//	btmTmp2.setVertexs(retFs);
		//	SLAM_LYJ::writePLYMesh("D:/tmp/checkF.ply", btmTmp2);
		//}
		//{
		//	std::vector<Eigen::Vector3f> fccc;
		//	for (int i = 0; i < h; ++i)
		//	{
		//		for (int j = 0; j < w; ++j)
		//		{
		//			const uint32_t& fid = fIdsOut[i * w + j];
		//			if (fid == UINT_MAX)
		//				continue;
		//			fccc.push_back(fCenters[fid]);
		//		}
		//	}
		//	SLAM_LYJ::BaseTriMesh btmtmp;
		//	btmtmp.setVertexs(fccc);
		//	SLAM_LYJ::writePLYMesh("D:/tmp/fccc.ply", btmtmp);
		//}
		{
			//std::vector<Eigen::Vector3f> PcsTmp;
			//Eigen::Vector2d uvTmp;
			cv::Mat mmmd(h, w, CV_8UC1);
			for (int i = 0; i < h; ++i)
			{
				for (int j = 0; j < w; ++j)
				{
					//Eigen::Vector3d Pc;
					double dd = depthsOut[i * w + j];
					//uvTmp(0) = j;
					//uvTmp(1) = i;
					//if (dd > 0.0f && dd != FLT_MAX)
					//{
					//	cam.image2World(uvTmp, dd, Pc);
					//	PcsTmp.push_back(Pc.cast<float>());
					//}
					const float ddd = depthsOut[i * w + j] / 30.0f;
					int dddc = ddd * 255 > 255 ? 255 : ddd * 255;
					mmmd.at<uchar>(i, j) = (uchar)dddc;
				}
			}
			//SLAM_LYJ::BaseTriMesh btmtmp;
			//btmtmp.setVertexs(PcsTmp);
			//SLAM_LYJ::writePLYMesh("D:/tmp/PcsTmp.ply", btmtmp);
			//cv::imwrite("D:/tmp/depth.png", mmmd);
			cv::imshow("dVK", mmmd);
			cv::waitKey();
		}
	}

	projectVK.release();
}


int main()
{
	// testVulkanCompute();
	//testVulkanGraphic();
	testProject();
}
