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
	VKPipelineAbr() = delete;
	VKPipelineAbr(int _cnt);
	~VKPipelineAbr();

	bool setBufferBinding(const int _binding, VKBufferAbr* _buffer, int _cnti=0);
	VkResult build();
	virtual void destroy();

	//tmp
	inline VkPipeline getPipeline() { return m_pipeline; }
	inline VkDescriptorSet& getDescriptorSet(int _i = 0) { return m_descriptorSets[_i]; }
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
	virtual VkResult createPipeline() = 0;
	VkResult createVKDescriptorPool();
	VkResult createVKDescriptorSetLayout();
	VkResult createVKPipelineLayout();
	void createVKDescriptorSets();
	VkResult createVKPipelineCahce();

protected:
	VkDevice m_device = VK_NULL_HANDLE;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

	int m_cnt = 0;
	std::vector<VkDescriptorSet> m_descriptorSets;
	std::vector<std::vector<int>> m_locations; //every std::vector<int> is same
	std::vector<std::vector<VKBufferAbr*>> m_buffers;
	std::vector<std::vector<VkDescriptorSetLayoutBinding>> m_layoutBindings; //size == 1
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
	VkResult createPipeline() override;

private:
	std::string m_path = "";
	uint32_t m_blockx = 1;
	uint32_t m_blocky = 1;
	uint32_t m_blockz = 1;
};




class VULKAN_LYJ_API ClassResolver
{
public:
	ClassResolver();
	~ClassResolver();
	void addBindingDescriptor(uint32_t _binding, uint32_t _stride, VkVertexInputRate _inputRate = VK_VERTEX_INPUT_RATE_VERTEX);
	void addAttributeDescriptor(uint32_t _binding, uint32_t _location, VkFormat _format, uint32_t _offset);
	std::vector<VkVertexInputBindingDescription> vertexInputBindings;
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
};

class VULKAN_LYJ_API VKFrameBuffer
{
public:
	VKFrameBuffer(uint32_t _width, uint32_t _height);
	~VKFrameBuffer();
	inline VkFramebuffer getFrameBuffer() { return m_frameBuffer; };

	VkResult create(VkRenderPass _renderPass, std::vector<VkImageView>& _imageViews);
	void destroy();
private:
	VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
	uint32_t m_width;
	uint32_t m_height;
	VkDevice m_device = VK_NULL_HANDLE;
};

class VULKAN_LYJ_API VKPipelineGraphics : public VKPipelineAbr
{
public:
	VKPipelineGraphics() = delete;
	VKPipelineGraphics(const std::string& _vertShaderPath, const std::string& _fragShaderPath, uint32_t _imageCnt);
	~VKPipelineGraphics();

	void setVertexBuffer(VKBufferVertex* _vertexBuffer, uint32_t _verCnt, ClassResolver& _classResolver);
	void setIndexBuffer(VKBufferIndex* _indexBuffer, uint32_t _indexCnt);
	void setImage(int _cnti, int _atti, std::shared_ptr<VKBufferImage>& _image);
	inline std::shared_ptr<VKFrameBuffer> getFrameBuffer(int _i) { return m_framebuffers[_i]; };
	inline VkRenderPass getRenderPass() { return m_renderPass; };
	inline void setCurId(int _curId) { m_curId = _curId; }

	// 通过 VKPipelineAbr 继承
	void destroy() override;

	// 通过 VKCommandAbr 继承
	void record(VkCommandBuffer _cmdBuffer) override;
private:
	// 通过 VKPipelineAbr 继承
	VkResult createPipeline() override;

	VkResult createRenderPass();
	VkResult createFrameBuffers();
private:
	std::string m_vertShaderPath = "";
	std::string m_fragShaderPath = "";
	VKBufferAbr* m_vertexBuffer = nullptr;
	VKBufferAbr* m_indexBuffer = nullptr;
	uint32_t m_indexCount = 0;
	uint32_t m_vertexCount = 0;
	ClassResolver m_classResolver;
	VkExtent2D m_extent{};
	VkClearValue m_clearColor{ 1.f, 0.f, 0.f, 1.f };

	//renderpass
	VkRenderPass m_renderPass = VK_NULL_HANDLE;

	//pipeline other
	std::vector<VkDynamicState> m_dynamicStates;
	std::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachmentStates;
	std::vector<VkViewport> m_viewports;
	std::vector<VkRect2D> m_scissors;

	//run
	int m_curId = 0;

	//target
	std::vector<std::vector<std::shared_ptr<VKBufferImage>>> m_images;//every std::vector<VkFormat> is same
	std::vector<int> m_attachLocations;//every std::vector<VkFormat> is same
	std::vector<std::shared_ptr<VKFrameBuffer>> m_framebuffers;
};





NSP_VULKAN_LYJ_END

#endif // VULKAN_PIPELINE_H