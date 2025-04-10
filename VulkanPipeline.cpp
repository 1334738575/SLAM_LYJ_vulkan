#include "VulkanPipeline.h"


NSP_VULKAN_LYJ_BEGIN

VKPipelineAbr::VKPipelineAbr(int _cnt):m_cnt(_cnt)
{
	m_device = GetLYJVKInstance()->m_device;
	m_descriptorSets.resize(m_cnt);
	m_locations.resize(m_cnt);
	m_buffers.resize(m_cnt);
	m_layoutBindings.resize(m_cnt);
}
VKPipelineAbr::~VKPipelineAbr()
{
	//destroy();
}
bool VKPipelineAbr::setBufferBinding(const int _binding, VKBufferAbr* _buffer, int _cnti)
{
	if (m_cnt <= 0 || _cnti >= m_cnt)
		return false;
	if (!m_locations[_cnti].empty())
		if (std::find(m_locations[_cnti].begin(), m_locations[_cnti].end(), _binding) != m_locations[_cnti].end())
			return false;
	m_locations[_cnti].push_back(_binding);
	m_buffers[_cnti].push_back(_buffer);
	return true;
}
VkResult VKPipelineAbr::build()
{
	VK_CHECK_RESULT(createVKDescriptorPool());
	VK_CHECK_RESULT(createVKDescriptorSetLayout());
	VK_CHECK_RESULT(createVKPipelineLayout());
	VK_CHECK_RESULT(createVKPipelineCahce());
	createVKDescriptorSets();
	return createPipeline();
}
void VKPipelineAbr::destroy()
{
	if (m_pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(m_device, m_pipeline, nullptr);
	if (m_pipelineCache != VK_NULL_HANDLE)
		vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);
	if (m_descriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	if (m_descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
	if (m_pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}
VkResult VKPipelineAbr::createVKDescriptorPool()
{
	int descCnt = m_buffers[0].size();
	std::vector<VkDescriptorPoolSize> descriptorTypeCounts(descCnt);
	for (int cnt = 0; cnt < descCnt; ++cnt) {
		const auto& bfType = m_buffers[0][cnt]->getType();
		if (bfType == VKBufferAbr::BUFFERTYPE::UNIFORM) {
			descriptorTypeCounts[cnt].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorTypeCounts[cnt].descriptorCount = m_cnt;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::TEXTURE && cnt == 0) {
			descriptorTypeCounts[cnt].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorTypeCounts[cnt].descriptorCount = m_cnt;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::TEXTURE) {
			descriptorTypeCounts[cnt].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			descriptorTypeCounts[cnt].descriptorCount = m_cnt;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::COMPUTE) {
			descriptorTypeCounts[cnt].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorTypeCounts[cnt].descriptorCount = m_cnt;
		}
	}
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.poolSizeCount = (uint32_t)descriptorTypeCounts.size();
	descriptorPoolInfo.pPoolSizes = descriptorTypeCounts.data();
	descriptorPoolInfo.maxSets = m_cnt;
	return vkCreateDescriptorPool(m_device, &descriptorPoolInfo, nullptr, &m_descriptorPool);
}
VkResult VKPipelineAbr::createVKDescriptorSetLayout()
{
	int descCnt = m_buffers[0].size();
	m_layoutBindings[0].resize(descCnt);
	for (int cnt = 0; cnt < descCnt; ++cnt) {
		const auto& bfType = m_buffers[0][cnt]->getType();
		m_layoutBindings[0][cnt].descriptorCount = 1;
		m_layoutBindings[0][cnt].pImmutableSamplers = nullptr;
		m_layoutBindings[0][cnt].binding = m_locations[0][cnt];
		if (bfType == VKBufferAbr::BUFFERTYPE::UNIFORM) {
			m_layoutBindings[0][cnt].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			m_layoutBindings[0][cnt].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::TEXTURE && cnt == 0 ) {
			m_layoutBindings[0][cnt].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			m_layoutBindings[0][cnt].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::TEXTURE) {
			m_layoutBindings[0][cnt].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			m_layoutBindings[0][cnt].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::COMPUTE) {
			m_layoutBindings[0][cnt].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			m_layoutBindings[0][cnt].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		}
		else {
			return VkResult::VK_ERROR_UNKNOWN;
		}
	}
	VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
	descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutInfo.pNext = nullptr;
	descriptorLayoutInfo.bindingCount = (uint32_t)m_layoutBindings[0].size();
	descriptorLayoutInfo.pBindings = m_layoutBindings[0].data();
	return vkCreateDescriptorSetLayout(m_device, &descriptorLayoutInfo, nullptr, &m_descriptorSetLayout);
}
VkResult VKPipelineAbr::createVKPipelineLayout()
{
	std::vector<VkDescriptorSetLayout> layouts = { m_descriptorSetLayout };
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
	pipelineLayoutCreateInfo.setLayoutCount = (uint32_t)layouts.size();
	return vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
}
void VKPipelineAbr::createVKDescriptorSets()
{
	for (int di = 0; di < m_cnt; ++di) {
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &m_descriptorSetLayout;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, &m_descriptorSets[di]));
		int descCnt = m_buffers[di].size();
		std::vector< VkWriteDescriptorSet> writeDescriptorSets(descCnt);
		for (int i = 0; i < descCnt; ++i) {
			writeDescriptorSets[i] = VkWriteDescriptorSet{};
			writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[i].descriptorCount = 1;
			writeDescriptorSets[i].dstSet = m_descriptorSets[di];
			writeDescriptorSets[i].descriptorType = m_layoutBindings[0][i].descriptorType;
			writeDescriptorSets[i].dstBinding = m_layoutBindings[0][i].binding;
			if (m_layoutBindings[0][i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
				m_layoutBindings[0][i].descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
				writeDescriptorSets[i].pImageInfo = m_buffers[di][i]->getImageInfo();
			else
				writeDescriptorSets[i].pBufferInfo = m_buffers[di][i]->getBufferInfo();
		}
		vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	}
	return;
}
VkResult VKPipelineAbr::createVKPipelineCahce()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	return vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
}




VKPipelineCompute::VKPipelineCompute(const std::string& _path) : VKPipelineAbr(1), m_path(_path)
{
	m_type = CMDTYPE::COMPUTE;
}
VKPipelineCompute::~VKPipelineCompute()
{}
void VKPipelineCompute::setRunKernel(uint32_t _lenx, uint32_t _leny, uint32_t _lenz,
	uint32_t _localx, uint32_t _localy, uint32_t _localz)
{
	m_blockx = (_lenx + _localx - 1) / _localx;
	m_blocky = (_leny + _localy - 1) / _localy;
	m_blockz = (_lenz + _localz - 1) / _localz;
}
void VKPipelineCompute::record(VkCommandBuffer _cmdBuffer)
{
	vkCmdBindPipeline(_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
	vkCmdBindDescriptorSets(_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSets[0], 0, nullptr);
	vkCmdDispatch(_cmdBuffer, m_blockx, m_blocky, m_blockz);
}
VkResult VKPipelineCompute::createPipeline()
{
	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = m_pipelineLayout;
	computePipelineCreateInfo.flags = 0;
	auto computeShaderCode = readFile(m_path);
	VkShaderModule computeShaderModule = createShaderModule(m_device, computeShaderCode);
	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{};
	pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineShaderStageCreateInfo.module = computeShaderModule;
	pipelineShaderStageCreateInfo.pName = "main";
	assert(pipelineShaderStageCreateInfo.module != VK_NULL_HANDLE);
	computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
	VkResult ret = vkCreateComputePipelines(m_device, nullptr, 1, &computePipelineCreateInfo, nullptr, &m_pipeline);
	vkDestroyShaderModule(m_device, computeShaderModule, nullptr);
	return ret;
}




ClassResolver::ClassResolver()
{}
ClassResolver::~ClassResolver()
{}
void ClassResolver::addBindingDescriptor(uint32_t _binding, uint32_t _stride, VkVertexInputRate _inputRate)
{
	VkVertexInputBindingDescription vertexInputBinding{};
	vertexInputBinding.binding = _binding;
	vertexInputBinding.stride = _stride;
	vertexInputBinding.inputRate = _inputRate;
	vertexInputBindings.push_back(vertexInputBinding);
}
void ClassResolver::addAttributeDescriptor(uint32_t _binding, uint32_t _location, VkFormat _format, uint32_t _offset)
{
	VkVertexInputAttributeDescription vertexInputAttribute{};
	vertexInputAttribute.binding = _binding;
	vertexInputAttribute.location = _location;
	vertexInputAttribute.format = _format;
	vertexInputAttribute.offset = _offset;
	vertexInputAttributes.push_back(vertexInputAttribute);
}

VKFrameBuffer::VKFrameBuffer(uint32_t _width, uint32_t _height)
	: m_width(_width), m_height(_height)
{
	m_device = GetLYJVKInstance()->m_device;
}
VKFrameBuffer::~VKFrameBuffer()
{}
VkResult VKFrameBuffer::create(VkRenderPass _renderPass, std::vector<VkImageView>& _imageViews)
{
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = _renderPass;
	framebufferInfo.attachmentCount = _imageViews.size();
	framebufferInfo.pAttachments = &_imageViews.front();
	framebufferInfo.width = m_width;
	framebufferInfo.height = m_height;
	framebufferInfo.layers = 1;
	m_clrAttCnt = _imageViews.size();
	return vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_frameBuffer);
}
void VKFrameBuffer::destroy()
{
	if(m_frameBuffer)
		vkDestroyFramebuffer(m_device, m_frameBuffer, nullptr);
}

VKPipelineGraphics::VKPipelineGraphics(const std::string& _vertShaderPath, const std::string& _fragShaderPath, uint32_t _imageCnt)
	: VKPipelineAbr(_imageCnt), m_vertShaderPath(_vertShaderPath), m_fragShaderPath(_fragShaderPath)
{
	m_type = CMDTYPE::GRAPHICS;
	m_images.resize(_imageCnt);
	m_framebuffers.resize(_imageCnt);
}
inline VKPipelineGraphics::~VKPipelineGraphics()
{}
void VKPipelineGraphics::setVertexBuffer(VKBufferVertex * _vertexBuffer, uint32_t _verCnt, ClassResolver& _classResolver)
{
	m_vertexBuffer = _vertexBuffer;
	m_vertexCount = _verCnt;
	m_classResolver = _classResolver;
}
void VKPipelineGraphics::setIndexBuffer(VKBufferIndex* _indexBuffer, uint32_t _indexCnt)
{
	m_indexBuffer = _indexBuffer;
	m_indexCount = _indexCnt;
}
void VKPipelineGraphics::setImage(int _cnti, int _atti, std::shared_ptr<VKBufferImage>& _image)
{
	m_images[_cnti].push_back(_image);
	m_attachLocations.push_back(_atti);
	//if (_atti > 0)
	//	setBufferBinding(_atti, _image.get(), _cnti);
}
std::shared_ptr<VKBufferImage> VKPipelineGraphics::getImage(int _cnti, int _atti)
{
	return m_images[_cnti][_atti];
}
VkResult VKPipelineGraphics::createPipeline()
{
	createRenderPass();
	createFrameBuffers();
	auto vertShaderCode = LYJ_VK::VKPipelineAbr::readFile(m_vertShaderPath);
	auto fragShaderCode = LYJ_VK::VKPipelineAbr::readFile(m_fragShaderPath);
	VkShaderModule vertShaderModule = LYJ_VK::VKPipelineAbr::createShaderModule(m_device, vertShaderCode);
	VkShaderModule fragShaderModule = LYJ_VK::VKPipelineAbr::createShaderModule(m_device, fragShaderCode);
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.layout = m_pipelineLayout;
	pipelineCreateInfo.renderPass = m_renderPass;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = (uint32_t)m_classResolver.vertexInputBindings.size();
	vertexInputStateCreateInfo.pVertexBindingDescriptions = m_classResolver.vertexInputBindings.data();
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)m_classResolver.vertexInputAttributes.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = m_classResolver.vertexInputAttributes.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;

	VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{}; //图元细分阶段
	tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	pipelineCreateInfo.pTessellationState = &tessellationStateCreateInfo;

	VkViewport vp{ 0.f, 0.f, float(m_extent.width), float(m_extent.height), 0.f, 1.f };
	m_viewports.push_back(vp);
	VkOffset2D of2d{ 0,0 };
	VkRect2D sr{ of2d, m_extent };
	m_scissors.push_back(sr);
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = (uint32_t)m_viewports.size();
	viewportStateCreateInfo.pViewports = m_viewports.data();
	viewportStateCreateInfo.scissorCount = (uint32_t)m_scissors.size();
	viewportStateCreateInfo.pScissors = m_scissors.data();
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;

	VkPipelineMultisampleStateCreateInfo multistampleStateCreateInfo{};
	multistampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multistampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineCreateInfo.pMultisampleState = &multistampleStateCreateInfo;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;

	VkPipelineColorBlendAttachmentState tmp{};
	tmp.colorWriteMask = 0b1111;
	m_colorBlendAttachmentStates.push_back(tmp);
	m_colorBlendAttachmentStates.push_back(tmp);
	m_colorBlendAttachmentStates.push_back(tmp);
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.attachmentCount = (uint32_t)m_colorBlendAttachmentStates.size();
	colorBlendStateCreateInfo.pAttachments = m_colorBlendAttachmentStates.data();
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;

	//m_dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	//m_dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = (uint32_t)m_dynamicStates.size();
	dynamicStateCreateInfo.pDynamicStates = m_dynamicStates.data();
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

	VkResult ret = vkCreateGraphicsPipelines(m_device, nullptr, 1, &pipelineCreateInfo, nullptr, &m_pipeline);
	vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
	return ret;
}
VkResult VKPipelineGraphics::createRenderPass()
{
	if (m_images.empty())
		return VK_SUCCESS;
	uint32_t attachSize = m_images[0].size();
	if (attachSize == 0)
		return VK_SUCCESS;

	//附件所有子通道共享
	std::vector<VkAttachmentDescription> colorAttachments(attachSize);
	for (int i = 0; i < attachSize; ++i) {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_images[0][i]->getFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if(m_images[0][i]->getType() == VKBufferAbr::BUFFERTYPE::TEXTURE)
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		if (m_images[0][i]->getType() == VKBufferAbr::BUFFERTYPE::DEPTH)
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		colorAttachments[i] = colorAttachment;
	}

	//附件引用
	std::vector<VkAttachmentReference> attachmentRefs(attachSize);
	for (int i = 0; i < attachSize; ++i) {
		VkAttachmentReference attachmentRef{};
		attachmentRef.attachment = i;
		attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentRefs[i] = attachmentRef;
	}

	//子通道
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = attachmentRefs.size();
	subpass.pColorAttachments = &attachmentRefs.front();
	//subpass.pDepthStencilAttachment;

	//子通道之间的依赖
	//VkSubpassDependency dependency{};
	//dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	//dependency.dstSubpass = 0;
	//dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//dependency.srcAccessMask = 0;
	//dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	std::array<VkSubpassDependency, 2> dependencies;
	// This makes sure that writes to the depth image are done before we try to write to it again
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = 0;
	dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].dstSubpass = 0;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].srcAccessMask = 0;
	dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dependencyFlags = 0;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachSize;
	renderPassInfo.pAttachments = &colorAttachments.front();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	return vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
}
VkResult VKPipelineGraphics::createFrameBuffers()
{
	m_extent.width = m_images[0][0]->getWidth();
	m_extent.height = m_images[0][0]->getHeight();
	for (size_t i = 0; i < m_cnt; ++i) {
		int imgSize = m_images[i].size();
		m_framebuffers[i].reset(new LYJ_VK::VKFrameBuffer(m_images[i][0]->getWidth(), m_images[i][0]->getHeight()));
		std::vector<VkImageView> attachments(imgSize);
		for (size_t j = 0; j < imgSize; ++j)
			attachments[j] = m_images[i][j]->getImageInfo()->imageView;
		if (m_framebuffers[i]->create(m_renderPass, attachments) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer");
	}
	return VK_SUCCESS;
}
void VKPipelineGraphics::destroy()
{
	if (m_pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(m_device, m_pipeline, nullptr);
	if (m_pipelineCache != VK_NULL_HANDLE)
		vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);
	if (m_descriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	if (m_descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
	if (m_pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	if (m_renderPass)
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	for (auto framebuffer : m_framebuffers)
		if(framebuffer)
			framebuffer->destroy();
}
void VKPipelineGraphics::record(VkCommandBuffer _cmdBuffer)
{
	uint32_t clrAttCnt = m_framebuffers.at(m_curId)->getColorAttachmentCount();
	m_clearColors.resize(clrAttCnt);
	for (uint32_t i = 0; i < clrAttCnt; ++i) {
		m_clearColors[i] = VkClearValue{};
		m_clearColors[i].color = { 1.f, 0.f, 0.f, 1.f };
	}
	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.framebuffer = m_framebuffers.at(m_curId)->getFrameBuffer();
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_extent.width;
	renderPassBeginInfo.renderArea.extent.height = m_extent.height;
	renderPassBeginInfo.clearValueCount = clrAttCnt;
	renderPassBeginInfo.pClearValues = &m_clearColors.front();
	vkCmdBeginRenderPass(_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindDescriptorSets(_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_curId], 0, nullptr);
	vkCmdBindPipeline(_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
	//vkCmdDraw(m_cmdBuffer, 3, 1, 0, 0);
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(_cmdBuffer, 0, 1, &m_vertexBuffer->getBuffer(), offsets);
	vkCmdBindIndexBuffer(_cmdBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(_cmdBuffer, m_indexCount, 1, 0, 0, 0);

	vkCmdEndRenderPass(_cmdBuffer);


}



NSP_VULKAN_LYJ_END
