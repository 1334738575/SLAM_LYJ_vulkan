#include "test.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"

static std::string shaderPath = "D:/testLyj/Vulkan/shader/";
static std::string imagePath = "D:/SLAM_LYJ/other/";


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}


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
	LYJ_VK::VKInstance* lyjVK = LYJ_VK::GetLYJVKInstance();
	if (!lyjVK->isInited()) {
		if (lyjVK->init(false, nullptr, true) != VK_SUCCESS) {
			std::cout << "init vulkan fail!" << std::endl;
			return false;
		}
	}
	return true;
}
void VKComputeTest::run()
{
	LYJ_VK::VKInstance* lyjVK = LYJ_VK::GetLYJVKInstance();
	VkDevice device = lyjVK->m_device;
	VkQueue computeQueue = lyjVK->m_computeQueues[0];
	VkCommandPool computeCommandPool = lyjVK->m_computeCommandPool;

	//generate data
	const int computeSize = 32;
	const VkDeviceSize bufferSize = computeSize * sizeof(uint32_t);
	uint32_t n = 0;
	std::vector<uint32_t> computeInput(computeSize);
	std::vector<uint32_t> computeOutput(computeSize);
	std::generate(computeInput.begin(), computeInput.end(), [&n] {return n++; });
	std::vector<uint32_t> computeInput2(computeSize);
	std::vector<uint32_t> computeOutput2(computeSize);
	uint32_t n2 = 5;
	std::generate(computeInput2.begin(), computeInput2.end(), [&n2] {return n2++; });
	const VkDeviceSize bufferSize2 = computeSize * sizeof(uint32_t);

	//upload
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

	//pipeline
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
		{ m_devBuffer->getBuffer(), m_devBuffer2->getBuffer() },
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	LYJ_VK::VKCommandBufferBarrier cmdBufferBarrier2(
		{ m_devBuffer->getBuffer(), m_devBuffer2->getBuffer() },
		VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	LYJ_VK::VKCommandMemoryBarrier endBarrier;
	m_imp.reset(new LYJ_VK::VKImp(0));
	m_imp->setCmds({ &cmdBufferBarrier, m_com1.get(), &cmdBufferBarrier2, m_com2.get(), &endBarrier });

	//compute
	LYJ_VK::VKFence fence;
	m_imp->run(computeQueue, fence.ptr());
	fence.wait();
	fence.reset();

	//release copy buffer
	m_devBuffer2->releaseBufferCopy();
	m_devBuffer->releaseBufferCopy();

	//download
	//uint32_t* retPtr2 = (uint32_t*)m_devBuffer2->download(bufferSize, computeOutput2.data(), computeQueue);
	//uint32_t* retPtr = (uint32_t*)m_devBuffer->download(bufferSize, computeOutput.data(), computeQueue);
	uint32_t* retPtr2 = (uint32_t*)m_devBuffer2->download(bufferSize, computeQueue);
	uint32_t* retPtr = (uint32_t*)m_devBuffer->download(bufferSize, computeQueue, fence.ptr());
	fence.wait();
	memcpy(computeOutput2.data(), retPtr2, bufferSize);
	memcpy(computeOutput.data(), retPtr, bufferSize);
	m_devBuffer2->releaseBufferCopy();
	m_devBuffer->releaseBufferCopy();

	// Output
	std::cout << "Compute input:\n";
	for (auto v : computeInput) {
		std::cout << v << " ";
	}
	std::cout << std::endl;
	std::cout << "Compute output:\n";
	for (auto v : computeOutput) {
		std::cout << v << " ";
	}
	std::cout << std::endl;
	for (auto v : computeOutput2) {
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
	if (m_bPresent) {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	}
	m_lyjVK = LYJ_VK::GetLYJVKInstance();
	if (!m_lyjVK->isInited()) {
		if (m_lyjVK->init(m_bPresent, nullptr, true) != VK_SUCCESS) {
			std::cout << "init vulkan fail!" << std::endl;
			return;
		}
	}
}
VKGraphicTest::~VKGraphicTest()
{
	cleanup();
}
void VKGraphicTest::run() {
	init();
	if (m_bPresent)
		mainLoop();
	else
		drawFrame();
}
void VKGraphicTest::init() {
	m_pipelineGraphics.reset(new LYJ_VK::VKPipelineGraphics(shaderPath + "texture/texture.vert.spv", shaderPath + "texture/texture.frag.spv", m_imgSize));
	if(m_bPresent)
		m_swapChain.reset(new LYJ_VK::VKSwapChain(m_imgSize));

	//texture2d
	auto funcCreateTextureImage = [&]() {
		VkDevice device = m_lyjVK->m_device;
		VkCommandPool graphicsCommandPool = m_lyjVK->m_graphicsCommandPool;
		VkQueue graphicQueue = m_lyjVK->m_graphicQueues[0];
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties = m_lyjVK->m_memProperties;
		uint32_t imgCnt = m_imgSize;

		std::string imgName = imagePath + "IMG_9179[1](1).png";
		cv::Mat image1 = cv::imread(imgName, 0);
		cv::Mat image2 = image1.clone();
		if (image1.empty() || image2.empty()) {
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
		//m_image->upload(width* height * 4, image.data, graphicQueue);
		LYJ_VK::VKFence fence;
		m_image->upload(width * height * 4, image.data, graphicQueue, fence.ptr());
		fence.wait();
		m_image->releaseBufferCopy();
		for (int i = 0; i < imgCnt; ++i)
			m_pipelineGraphics->setBufferBinding(3, m_image.get(), i);
		};
	//funcCreateTextureImage();
	//return;

	//vertices and indices buffer
	const float edgeRatio = 0.5;
	const float textureEdgeRatio = 1.0;
	auto funcGenerateQuad = [&]() {
		VkQueue graphicQueue = m_lyjVK->m_graphicQueues[0];
		std::vector<Vertex> vertices =
		{
			{{edgeRatio, edgeRatio, 0.5f}, {textureEdgeRatio, textureEdgeRatio}, {0.f, 0.f, 1.f}},
			{{-edgeRatio, edgeRatio, 0.5f}, {0.f, textureEdgeRatio}, {0.f, 0.f, 1.f}},
			{{-edgeRatio, -edgeRatio, 0.5f}, {0.f, 0.f}, {0.f, 0.f, 1.f}},
			{{edgeRatio, -edgeRatio, 0.5f}, {textureEdgeRatio, 0.f}, {0.f, 0.f, 1.f}}
		};
		std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
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
	//return;

	//uniform buffer
	auto funcCreateUniformBuffers = [&]() {
		uint32_t imageCnt = m_imgSize;
		m_uniBuffers.resize(imageCnt, nullptr);
		for (uint32_t i = 0; i < m_uniBuffers.size(); ++i) {
			m_uniBuffers[i].reset(new LYJ_VK::VKBufferUniform());
			m_uniBuffers[i]->resize(sizeof(ShaderData));
			m_pipelineGraphics->setBufferBinding(0, m_uniBuffers[i].get(), i);
		}
		};
	funcCreateUniformBuffers();
	//return;

	//pipeline
	{
		if (m_bPresent) {
			std::vector<std::shared_ptr<LYJ_VK::VKBufferImage>>& images = m_swapChain->getImages();
			for (size_t i = 0; i < m_swapChain->getImageCnt(); ++i) {
				m_pipelineGraphics->setImage(i, 0, images[i]);
			}
		}
		int h = m_lyjVK->m_height;
		int w = m_lyjVK->m_width;
		for (size_t i = 0; i < m_imgSize; ++i) {
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
	//return;

	//imp
	m_imps.resize(m_imgSize, nullptr);
	for (int i = 0; i < m_imgSize; ++i) {
		m_imps[i].reset(new LYJ_VK::VKImp(0));
		m_imps[i]->setCmds({m_pipelineGraphics.get()});
	}
	//return;

	//fence and semaphore
	m_fence.reset(new LYJ_VK::VKFence());
	m_availableSemaphore.reset(new LYJ_VK::VKSemaphore());
	m_finishedSemaphore.reset(new LYJ_VK::VKSemaphore());
	m_cmdImgBarriers.resize(m_imgSize, nullptr);
	for (int i = 0; i < m_imgSize; ++i) {
		if(m_bPresent)
			m_cmdImgBarriers[i].reset(new LYJ_VK::VKCommandImageBarrier(
				{ m_pipelineGraphics->getImage(i, 0)->getImage() }, { m_pipelineGraphics->getImage(i,0)->getSubresource() },
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED));
		else
			m_cmdImgBarriers[i].reset(new LYJ_VK::VKCommandImageBarrier(
				{ m_pipelineGraphics->getImage(i, 0)->getImage() }, { m_pipelineGraphics->getImage(i,0)->getSubresource() },
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED));
	}
	return;
}
void VKGraphicTest::mainLoop() {
	VkDevice device = m_lyjVK->m_device;
	GLFWwindow* windows = m_lyjVK->m_windows;

	while (!glfwWindowShouldClose(windows)) {
		drawFrame();
		glfwPollEvents();
		m_fence->wait();
		m_fence->reset();
	}
	vkDeviceWaitIdle(device);
	glfwTerminate();
}
void VKGraphicTest::drawFrame() {
	VkDevice device = m_lyjVK->m_device;
	VkQueue graphicQueue = m_lyjVK->m_graphicQueues[0];
	VkQueue presentQueue = nullptr;
	VkSwapchainKHR swapChain = nullptr;
	uint32_t imageIndex = 0;

	if (m_bPresent) {
		presentQueue = m_lyjVK->m_presentQueues[0];
		swapChain = m_swapChain->getSwapChain();
		vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, m_availableSemaphore->ptr(), VK_NULL_HANDLE, &imageIndex);
		m_pipelineGraphics->setCurId(imageIndex);
	}
	ShaderData shaderData{};
	float xof = (m_cnt++ / 100) * 0.1f;
	//xof = 0.0f;
	shaderData.projectionMatrix = glm::mat4( //列为主
		1.0f, 0.0f, 0.0f, 0.0f, //第一列
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		xof, 0.0f, 0.0f, 1.0f
	);
	const VkExtent2D& extent2D = m_swapChain->getExtent2D();
	//shaderData.projectionMatrix = glm::perspective(glm::radians(-90.0f), (float)extent2D.width / (float)extent2D.width, 0.1f, 2.f);
	//shaderData.projectionMatrix *= -1;
	shaderData.viewMatrix = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	//shaderData.moduleMatrix = glm::mat4(1.0f);
	shaderData.moduleMatrix = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	m_uniBuffers[imageIndex]->upload(sizeof(ShaderData), &shaderData, graphicQueue);
	if (m_bPresent) {
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		m_imps[imageIndex]->run(graphicQueue, m_fence->ptr(), { m_availableSemaphore->ptr() }, { m_finishedSemaphore->ptr() }, waitStages);
	}
	else
		m_imps[imageIndex]->run(graphicQueue, m_fence->ptr());

	LYJ_VK::VKImp impTmp(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	impTmp.setCmds({ m_cmdImgBarriers[imageIndex].get()});
	LYJ_VK::VKFence fenceTmp;
	impTmp.run(graphicQueue, fenceTmp.ptr());
	fenceTmp.wait();
	impTmp.destroy();
	//get image in swapchain
	if (m_cnt && true) {
		int h = m_lyjVK->m_height;
		int w = m_lyjVK->m_width;

		{
			auto lyjImg = m_pipelineGraphics->getImage(0, 1 - int(!m_bPresent));
			int c = lyjImg->getChannels();
			int step = lyjImg->getStep();
			int s = w * h * c * step;
			LYJ_VK::VKFence fenceTmp;
			void* data = lyjImg->download(s, graphicQueue, fenceTmp.ptr());
			fenceTmp.wait();
			cv::Mat mmm(h, w, CV_8UC4);
			//cv::Mat mmm(h, w, CV_32FC4);
			memcpy(mmm.data, data, s);
			lyjImg->releaseBufferCopy();
			cv::imshow("111", mmm);
			//cv::waitKey();
		}
		{
			auto lyjImg = m_pipelineGraphics->getImage(0, 2 - int(!m_bPresent));
			int c = lyjImg->getChannels();
			int step = lyjImg->getStep();
			int s = w * h * c * step;
			LYJ_VK::VKFence fenceTmp;
			void* data = lyjImg->download(s, graphicQueue, fenceTmp.ptr());
			fenceTmp.wait();
			cv::Mat mmm(h, w, CV_32SC1);
			cv::Mat mmm2(h, w, CV_8UC1);
			memcpy(mmm.data, data, s);
			lyjImg->releaseBufferCopy();
			//mmm *= 127;
			for (int i = 0; i < h; ++i) {
				for (int j = 0; j < w; ++j) {
					int fid = mmm.at<int>(i, j);
					mmm2.at<uchar>(i, j) = (uchar)(fid * 127);
				}
			}
			cv::imshow("333", mmm2);
			//cv::waitKey();
		}
		{
			auto lyjDepth = m_pipelineGraphics->getDepthImage();
			int c = lyjDepth->getChannels();
			int step = lyjDepth->getStep();
			int s = w * h * c * step;
			LYJ_VK::VKFence fenceTmp;
			void* data = lyjDepth->download(s, graphicQueue, fenceTmp.ptr());
			fenceTmp.wait();
			cv::Mat mmm(h, w, CV_32FC1);
			memcpy(mmm.data, data, s);
			float* ppp = (float*)mmm.data;
			lyjDepth->releaseBufferCopy();
			cv::imshow("222", mmm);
			cv::waitKey();
		}
	}

	if (m_bPresent) {
		std::vector<VkSwapchainKHR> swapChains = { swapChain };
		std::vector<uint32_t> imageIndices = { imageIndex };
		std::vector<VkSemaphore> renderFinishedSemaphores = { m_finishedSemaphore->ptr() };
		LYJ_VK::VKImpPresent presentImp;
		if (presentImp.present(presentQueue, swapChains, imageIndices, renderFinishedSemaphores) != VK_SUCCESS) {
			throw std::runtime_error("failed to present image");
		}
	}
}
void VKGraphicTest::cleanup() {
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

	for(auto imp:m_imps)
		if (imp)
			imp->destroy();

	if (m_swapChain)
		m_swapChain->destory();

	m_lyjVK->clean();
}
