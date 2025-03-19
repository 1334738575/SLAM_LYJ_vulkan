#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#include "VulkanCommon.h"
#include <glm/glm.hpp>


NSP_VULKAN_LYJ_BEGIN
class VULKAN_LYJ_API VKBufferAbr
{
public:
	enum class BUFFERTYPE
	{
		DEFAULT,
		TRANSFER,
		UNIFORM,
		COMPUTE,
		VERTEX,
		INDEX,
		TEXTURE,
		DEPTH
	};
	VKBufferAbr();
	~VKBufferAbr();

	void resize(VkDeviceSize _size);
	virtual void upload(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE)=0;
	virtual void download(VkDeviceSize _size, void* _data, VkQueue _queue = VK_NULL_HANDLE)=0;
	void destroy(bool _bf=true, bool _mem=true);
	inline VkBuffer& getBuffer() { return m_buffer; };
	inline VkDescriptorBufferInfo* getBufferInfo() { return &m_bufferInfo; };
	inline VkDescriptorSet& getDescriptorSet() { return m_descriptorSet; };
	inline VkBufferUsageFlags& getBufferUsageFlags() { return m_usageFlags; };
	inline BUFFERTYPE getType() { return m_type; }

protected:
	VkResult createVkBuffer();
	VkResult createVkMemory();
	VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);//对device可见
	VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);//对host可见
	void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

protected:
	BUFFERTYPE m_type = BUFFERTYPE::DEPTH;
	VkDevice m_device = VK_NULL_HANDLE;
	VkBufferUsageFlags m_usageFlags{};
	VkMemoryPropertyFlags m_memoryPropertyFlags{};
	VkDeviceSize m_size = 0;
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VkDeviceSize m_capacity = 0;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;

	VkDescriptorBufferInfo m_bufferInfo{};
	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
};




class VULKAN_LYJ_API VKBufferTrans : public VKBufferAbr
{
public:
	VKBufferTrans();
	~VKBufferTrans();

	// 通过 VKBufferAbr 继承
	void upload(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE) override;
	void download(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE) override;

private:
	VkResult mapGPU2CPU(VkDeviceSize _size = VK_WHOLE_SIZE, VkDeviceSize _offset = 0);
	void copyTo(void* _data, VkDeviceSize _size, bool _isFromoCPU);
	void unmapGPU2CPU();

private:
	void* m_mapped = nullptr;
};




class VULKAN_LYJ_API VKBufferDevice : public VKBufferAbr
{
public:
	VKBufferDevice();
	~VKBufferDevice();

	// 通过 VKBufferAbr 继承
	void upload(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE) override;
	void download(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE) override;

private:


protected:
	std::shared_ptr<VKBufferTrans> m_bufferCopy = nullptr;
};

class VULKAN_LYJ_API VKBufferCompute : public VKBufferDevice
{
public:
	VKBufferCompute();
	~VKBufferCompute();

private:

};

class VULKAN_LYJ_API VKBufferUniform : public VKBufferDevice
{
public:
	VKBufferUniform();
	~VKBufferUniform();

private:

};

class VULKAN_LYJ_API VKBufferVertex : public VKBufferDevice
{
public:
	VKBufferVertex();
	~VKBufferVertex();

private:

};

class VULKAN_LYJ_API VKBufferIndex : public VKBufferDevice
{
public:
	VKBufferIndex();
	~VKBufferIndex();

private:

};




NSP_VULKAN_LYJ_END


#endif // !VULKAN_BUFFER_H

