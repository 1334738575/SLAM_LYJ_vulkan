#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include "VulkanCommon.h"
#include "VulkanBuffer.h"
#include "VulkanCommand.h"
#include <fstream>

NSP_VULKAN_LYJ_BEGIN


class VKPipelineAbr : public VKCommandAbr
{
public:
	VKPipelineAbr()=delete;
	VKPipelineAbr(const std::string& _path);
	~VKPipelineAbr();

	bool setBufferBinding(const int _binding, VKBufferAbr* _buffer);
	VkResult build();

	//tmp
	inline VkPipeline getPipeline() { return m_pipeline; }
	inline VkDescriptorSet& getDescriptorSet() { return m_descriptorSet; }
	inline VkPipelineLayout& getPipelineLayout() { return m_pipelineLayout; }

	static VkShaderModule createShaderModule(VkDevice& _device, const std::vector<char>& _code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = _code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(_code.data());
		VkShaderModule shaderModule;
		VK_CHECK_RESULT(vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule));
		return shaderModule;
	}
	static std::vector<char> readFile(const std::string& _filename) {
		std::ifstream file(_filename, std::ios::ate | std::ios::binary); //from file end
		if (!file.is_open())
			throw std::runtime_error("failed to open file");
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);//to file begin
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}

protected:
	virtual VkResult createPipeline(const std::string& _path) = 0;
	VkResult createVKDescriptorPool();
	VkResult createVKDescriptorSetLayout();
	VkResult createVKPipelineLayout();
	void createVKDescriptorSets();
	VkResult createVKPipelineCahce();

protected:
	VkDevice m_device = VK_NULL_HANDLE;
	std::string m_path = "";
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

	std::vector<int> m_locations;
	std::vector<VKBufferAbr*> m_buffers;
	std::vector<VkDescriptorSetLayoutBinding> m_layoutBindings;
};

VKPipelineAbr::VKPipelineAbr(const std::string& _path)
	:m_path(_path)
{
	m_device = GetLYJVKInstance()->m_device;
}
VKPipelineAbr::~VKPipelineAbr()
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
inline bool VKPipelineAbr::setBufferBinding(const int _binding, VKBufferAbr * _buffer)
{
	if (!m_locations.empty()) 
		if(std::find(m_locations.begin(), m_locations.end(), _binding) != m_locations.end())
			return false;
	m_locations.push_back(_binding);
	m_buffers.push_back(_buffer);
	return true;
}
inline VkResult VKPipelineAbr::build()
{
	VK_CHECK_RESULT(createVKDescriptorPool());
	VK_CHECK_RESULT(createVKDescriptorSetLayout());
	VK_CHECK_RESULT(createVKPipelineLayout());
	VK_CHECK_RESULT(createVKPipelineCahce());
	createVKDescriptorSets();
	return createPipeline(m_path);
}
inline VkResult VKPipelineAbr::createVKDescriptorPool()
{
	int descCnt = m_buffers.size();
	std::vector<VkDescriptorPoolSize> descriptorTypeCounts(descCnt);
	for (int cnt = 0; cnt < descCnt; ++cnt) {
		const auto& bfType = m_buffers[cnt]->getType();
		if (bfType == VKBufferAbr::BUFFERTYPE::UNIFORM) {
			descriptorTypeCounts[cnt].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorTypeCounts[cnt].descriptorCount = 1;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::TEXTURE) {
			descriptorTypeCounts[cnt].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorTypeCounts[cnt].descriptorCount = 1;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::COMPUTE) {
			descriptorTypeCounts[cnt].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorTypeCounts[cnt].descriptorCount = 1;
		}
	}
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = nullptr;
	descriptorPoolInfo.poolSizeCount = (uint32_t)descriptorTypeCounts.size();
	descriptorPoolInfo.pPoolSizes = descriptorTypeCounts.data();
	descriptorPoolInfo.maxSets = 1;
	return vkCreateDescriptorPool(m_device, &descriptorPoolInfo, nullptr, &m_descriptorPool);
}
inline VkResult VKPipelineAbr::createVKDescriptorSetLayout()
{
	int descCnt = m_buffers.size();
	m_layoutBindings.resize(descCnt);
	for (int cnt = 0; cnt < descCnt;++cnt) {
		const auto& bfType = m_buffers[cnt]->getType();
		m_layoutBindings[cnt].descriptorCount = 1;
		m_layoutBindings[cnt].pImmutableSamplers = nullptr;
		m_layoutBindings[cnt].binding = m_locations[cnt];
		if (bfType == VKBufferAbr::BUFFERTYPE::UNIFORM) {
			m_layoutBindings[cnt].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			m_layoutBindings[cnt].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::TEXTURE) {
			m_layoutBindings[cnt].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			m_layoutBindings[cnt].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		else if (bfType == VKBufferAbr::BUFFERTYPE::COMPUTE) {
			m_layoutBindings[cnt].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			m_layoutBindings[cnt].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		}
		else {
			return VkResult::VK_ERROR_UNKNOWN;
		}
	}
	VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
	descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutInfo.pNext = nullptr;
	descriptorLayoutInfo.bindingCount = (uint32_t)m_layoutBindings.size();
	descriptorLayoutInfo.pBindings = m_layoutBindings.data();
	return vkCreateDescriptorSetLayout(m_device, &descriptorLayoutInfo, nullptr, &m_descriptorSetLayout);
}
VkResult VKPipelineAbr::createVKPipelineLayout()
{
	std::vector<VkDescriptorSetLayout> layouts = { m_descriptorSetLayout};
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
	pipelineLayoutCreateInfo.setLayoutCount = (uint32_t)layouts.size();
	return vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
}
inline void VKPipelineAbr::createVKDescriptorSets()
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &m_descriptorSetLayout;
	VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, &m_descriptorSet));
	int descCnt = m_buffers.size();
	std::vector< VkWriteDescriptorSet> writeDescriptorSets(descCnt);
	for (int i = 0; i < descCnt;++i) {
		writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[i].descriptorCount = 1;
		writeDescriptorSets[i].dstSet = m_descriptorSet;
		writeDescriptorSets[i].descriptorType = m_layoutBindings[i].descriptorType;
		writeDescriptorSets[i].dstBinding = m_layoutBindings[i].binding;
		writeDescriptorSets[i].pBufferInfo = m_buffers[i]->getBufferInfo();
	}
	return vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
}

inline VkResult VKPipelineAbr::createVKPipelineCahce()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	return vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
}





class VKPipelineCompute : public VKPipelineAbr
{
public:
	VKPipelineCompute(const std::string& _path);
	~VKPipelineCompute();

private:
	// Í¨¹ý VKPipelineAbr ¼Ì³Ð
	VkResult createPipeline(const std::string& _path) override;
};
VKPipelineCompute::VKPipelineCompute(const std::string& _path) : VKPipelineAbr(_path)
{}
VKPipelineCompute::~VKPipelineCompute()
{}
VkResult VKPipelineCompute::createPipeline(const std::string & _path)
{
	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = m_pipelineLayout;
	computePipelineCreateInfo.flags = 0;
	auto computeShaderCode = readFile(_path);
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



NSP_VULKAN_LYJ_END

#endif // VULKAN_PIPELINE_H