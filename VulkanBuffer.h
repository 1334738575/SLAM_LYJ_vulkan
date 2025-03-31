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
	virtual void upload(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE, VkFence _fence=nullptr)=0;
	virtual void download(VkDeviceSize _size, void* _data, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr)=0;
	virtual void* download(VkDeviceSize _size, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) = 0;
	virtual void releaseBufferCopy()=0;
	virtual void destroy(bool _bf=true, bool _mem=true)=0;
	VkBuffer& getBuffer() { return m_buffer; };
	VkDescriptorBufferInfo* getBufferInfo() { return &m_bufferInfo; };
	VkImage& getImage() { return m_image; };
	VkDescriptorImageInfo* getImageInfo() { return &m_imageInfo; };
	VkBufferUsageFlags& getBufferUsageFlags() { return m_usageFlags; };
	VkDescriptorSet& getDescriptorSet() { return m_descriptorSet; };
	BUFFERTYPE getType() { return m_type; };
	VkDeviceSize getSize() { return m_size; };

protected:
	VkResult createVkBuffer();
	VkResult createVkMemory();
	VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);//对device可见
	VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);//对host可见
	void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

protected:
	BUFFERTYPE m_type = BUFFERTYPE::DEFAULT;
	VkDevice m_device = VK_NULL_HANDLE;
	VkBufferUsageFlags m_usageFlags{};
	VkMemoryPropertyFlags m_memoryPropertyFlags{};
	VkDeviceSize m_size = 0;
	VkDeviceSize m_capacity = 0;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;

	//buffer
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VkDescriptorBufferInfo m_bufferInfo{};

	//image
	VkImage m_image = VK_NULL_HANDLE;
	VkDescriptorImageInfo m_imageInfo{};

	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
};




class VULKAN_LYJ_API VKBufferTrans : public VKBufferAbr
{
public:
	VKBufferTrans();
	~VKBufferTrans();

	// 通过 VKBufferAbr 继承
	void upload(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void download(VkDeviceSize _size, void* _data, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void* download(VkDeviceSize _size, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void releaseBufferCopy() override {};
	void destroy(bool _bf = true, bool _mem = true) override;
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
	void upload(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void download(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void* download(VkDeviceSize _size, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void releaseBufferCopy() override;
	void destroy(bool _bf = true, bool _mem = true) override;
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

class VULKAN_LYJ_API VKBufferImage : public VKBufferAbr
{
public:
	VKBufferImage() = delete;
	VKBufferImage(uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step, VkFormat _format, BUFFERTYPE _type=BUFFERTYPE::TEXTURE);
	VKBufferImage(VkImage _image, VkImageView _imageView, uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step, VkFormat _format, BUFFERTYPE _type = BUFFERTYPE::TEXTURE); //for tmp
	~VKBufferImage();

	inline const uint32_t getWidth() const { return m_width; };
	inline const uint32_t getHeight() const { return m_height; };
	inline const uint32_t getChannels() const { return m_channels; };
	inline const uint32_t getStep() const { return m_step; };
	inline const VkFormat getFormat() const { return m_format; };
	inline const VkImageSubresourceRange& getSubresource() const { return m_subResourceRange; };

	// 通过 VKBufferAbr 继承
	void upload(VkDeviceSize _size, void* _data, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void download(VkDeviceSize _size, void* _data, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void* download(VkDeviceSize _size, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void releaseBufferCopy() override;
	void destroy(bool _bf = true, bool _mem = true) override;
private:


protected:
	std::shared_ptr<VKBufferTrans> m_bufferCopy = nullptr;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_channels = 0;
	uint32_t m_step = 1;
	VkFormat m_format;
	VkImageSubresourceRange m_subResourceRange{};
};




NSP_VULKAN_LYJ_END


#endif // !VULKAN_BUFFER_H

