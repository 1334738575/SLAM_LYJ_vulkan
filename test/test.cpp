#include "test.h"


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
		if (lyjVK->init() != VK_SUCCESS) {
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
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_lyjVK = LYJ_VK::GetLYJVKInstance();
	if (!m_lyjVK->isInited()) {
		if (m_lyjVK->init(true, nullptr, true) != VK_SUCCESS) {
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
	mainLoop();
}
void VKGraphicTest::init() {
	m_pipelineGraphics.reset(new LYJ_VK::VKPipelineGraphics(shaderPath + "texture/texture.vert.spv", shaderPath + "texture/texture.frag.spv", 2));
	m_swapChain.reset(new LYJ_VK::VKSwapChain(2));

	//render pass
	m_pipelineGraphics->addVkFormat(m_swapChain->getFormat());
	m_pipelineGraphics->setExtent2D(m_swapChain->getExtent2D());

	//texture2d
	auto funcCreateTextureImage = [&]() {
		VkDevice device = m_lyjVK->m_device;
		VkCommandPool graphicsCommandPool = m_lyjVK->m_graphicsCommandPool;
		VkQueue graphicQueue = m_lyjVK->m_graphicQueues[0];
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties = m_lyjVK->m_memProperties;
		uint32_t imgCnt = m_swapChain->getImageCnt();

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
		m_image.reset(new LYJ_VK::VKBufferImage(width, height, 4, 1, VK_FORMAT_R8G8B8A8_UNORM));
		//m_image->upload(width* height * 4, image.data, graphicQueue);
		LYJ_VK::VKFence fence;
		m_image->upload(width * height * 4, image.data, graphicQueue, fence.ptr());
		fence.wait();
		m_image->releaseBufferCopy();
		for (int i = 0; i < imgCnt; ++i)
			m_pipelineGraphics->setBufferBinding(1, m_image.get(), i);
		};
	funcCreateTextureImage();

	//vertices and indices buffer
	const float edgeRatio = 0.5;
	const float textureEdgeRatio = 1.0;
	auto funcGenerateQuad = [&]() {
		VkQueue graphicQueue = m_lyjVK->m_graphicQueues[0];
		std::vector<Vertex> vertices =
		{
			{{edgeRatio, edgeRatio, 0.f}, {textureEdgeRatio, textureEdgeRatio}, {0.f, 0.f, 1.f}},
			{{-edgeRatio, edgeRatio, 0.f}, {0.f, textureEdgeRatio}, {0.f, 0.f, 1.f}},
			{{-edgeRatio, -edgeRatio, 0.f}, {0.f, 0.f}, {0.f, 0.f, 1.f}},
			{{edgeRatio, -edgeRatio, 0.f}, {textureEdgeRatio, 0.f}, {0.f, 0.f, 1.f}}
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
		};
	funcGenerateQuad();

	//uniform buffer
	auto funcCreateUniformBuffers = [&]() {
		uint32_t imageCnt = m_swapChain->getImageCnt();
		m_uniBuffers.resize(imageCnt, nullptr);
		for (uint32_t i = 0; i < m_uniBuffers.size(); ++i) {
			m_uniBuffers[i].reset(new LYJ_VK::VKBufferUniform());
			m_uniBuffers[i]->resize(sizeof(ShaderData));
			m_pipelineGraphics->setBufferBinding(0, m_uniBuffers[i].get(), i);
		}
		};
	funcCreateUniformBuffers();

	//pipeline
	m_pipelineGraphics->build();

	//frame buffer
	auto funcCreateFramebuffers = [&]() {
		std::vector<VkImageView>& imageViews = m_swapChain->getImageViews();
		const VkExtent2D& extent = m_swapChain->getExtent2D();
		std::vector<std::shared_ptr<LYJ_VK::VKFrameBuffer>> framebuffers;
		framebuffers.resize(imageViews.size());
		for (size_t i = 0; i < imageViews.size(); ++i) {
			framebuffers[i].reset(new LYJ_VK::VKFrameBuffer(extent.width, extent.height));
			std::vector<VkImageView> attachments{ imageViews[i] };
			if (framebuffers[i]->create(m_pipelineGraphics->getRenderPass(), attachments) != VK_SUCCESS)
				throw std::runtime_error("failed to create framebuffer");
		}
		m_pipelineGraphics->setFrameBuffers(framebuffers);
		};
	funcCreateFramebuffers();

	//imp
	m_imp.reset(new LYJ_VK::VKImp(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
	m_imp->setCmds({ m_pipelineGraphics.get() });


	//fence and semaphore
	m_fence.reset(new LYJ_VK::VKFence());
	m_availableSemaphore.reset(new LYJ_VK::VKSemaphore());
	m_finishedSemaphore.reset(new LYJ_VK::VKSemaphore());
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
	VkQueue presentQueue = m_lyjVK->m_presentQueues[0];
	VkSwapchainKHR swapChain = m_swapChain->getSwapChain();

	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, m_availableSemaphore->ptr(), VK_NULL_HANDLE, &imageIndex);
	m_pipelineGraphics->setCurId(imageIndex);
	//get image in swapchain
	if(false){
		const VkExtent2D& extent2D = m_swapChain->getExtent2D();
		std::vector<VkImage>& vkImgs = m_swapChain->getImages();

		VkImage vkImg = vkImgs[0];
		int w = extent2D.width;
		int h = extent2D.height;
		int c = 4;
		int step = 1;
		int s = w * h * c * step;
		LYJ_VK::VKBufferImage lyjImg(vkImg, w, h, c, step, VK_FORMAT_R8G8B8A8_UNORM);
		LYJ_VK::VKFence fenceTmp;
		void* data = lyjImg.download(s, graphicQueue, fenceTmp.ptr());
		fenceTmp.wait();
		cv::Mat mmm(h, w, CV_8UC4);
		memcpy(mmm.data, data, s);
		lyjImg.releaseBufferCopy();
		cv::imshow("111", mmm);
		cv::waitKey();
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
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	m_imp->run(graphicQueue, m_fence->ptr(), { m_availableSemaphore->ptr() }, { m_finishedSemaphore->ptr() }, waitStages);

	std::vector<VkSwapchainKHR> swapChains = { swapChain };
	std::vector<uint32_t> imageIndices = { imageIndex };
	std::vector<VkSemaphore> renderFinishedSemaphores = { m_finishedSemaphore->ptr() };
	LYJ_VK::VKImpPresent presentImp;
	if (presentImp.present(presentQueue, swapChains, imageIndices, renderFinishedSemaphores) != VK_SUCCESS) {
		throw std::runtime_error("failed to present image");
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
	for (auto uniBuffer : m_uniBuffers)
		if (uniBuffer)
			uniBuffer->destroy();
	if (m_verBuffer)
		m_verBuffer->destroy();
	if (m_indBuffer)
		m_indBuffer->destroy();

	if (m_imp)
		m_imp->destroy();

	if (m_swapChain)
		m_swapChain->destory();

	m_lyjVK->clean();
}
