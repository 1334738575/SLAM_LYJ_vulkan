#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

#include "VulkanCommon.h"
#include "VulkanBuffer.h"
#include "VulkanCommand.h"
#include <fstream>

NSP_VULKAN_LYJ_BEGIN


class VULKAN_LYJ_API VKPipelineAbr : public VKCommandAbr
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







class VULKAN_LYJ_API VKPipelineCompute : public VKPipelineAbr
{
public:
	VKPipelineCompute(const std::string& _path);
	~VKPipelineCompute();

	void setRunKernel(uint32_t _lenx, uint32_t _leny=1, uint32_t _lenz=1,
		uint32_t _localx = 128, uint32_t _localy = 1, uint32_t _localz = 1);

	// 通过 VKCommandAbr 继承
	void record(VkCommandBuffer _cmdBuffer) override;
private:
	// 通过 VKPipelineAbr 继承
	VkResult createPipeline(const std::string& _path) override;

private:
	uint32_t m_blockx = 1;
	uint32_t m_blocky = 1;
	uint32_t m_blockz = 1;
};





class VULKAN_LYJ_API VKPipelineGraphics : public VKPipelineAbr
{
public:
	VKPipelineGraphics(const std::string& _path);
	~VKPipelineGraphics();


	// 通过 VKCommandAbr 继承
	void record(VkCommandBuffer _cmdBuffer) override;
private:
	// 通过 VKPipelineAbr 继承
	VkResult createPipeline(const std::string& _path) override;

};





NSP_VULKAN_LYJ_END

#endif // VULKAN_PIPELINE_H