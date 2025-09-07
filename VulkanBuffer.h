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
		COLOR,
		SAMPLER,
		DEPTH
	};
	VKBufferAbr();
	~VKBufferAbr();

	void resize(VkDeviceSize _size);
	virtual void upload(VkDeviceSize _size, void* _data, VkQueue _queue=VK_NULL_HANDLE, VkFence _fence=nullptr)=0;
	virtual void download(VkDeviceSize _size, void* _data, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr)=0;
	virtual void* download(VkDeviceSize _size, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) = 0;
	virtual void resetData(VkDeviceSize _size=0, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) = 0;
	virtual void releaseBufferCopy()=0;
	virtual void destroy(bool _bf=true, bool _mem=true)=0;
	VkBuffer& getBuffer() { return m_buffer; };
	VkDescriptorBufferInfo* getBufferInfo() { return &m_bufferInfo; };
	VkImage& getImage() { return m_image; };
	VkDescriptorImageInfo* getImageInfo() { return &m_imageInfo; };
	VkBufferUsageFlags& getBufferUsageFlags() { return m_bufferUsageFlags; };
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

	VkMemoryPropertyFlags m_memoryPropertyFlags{};
	VkDeviceSize m_size = 0;
	VkDeviceSize m_capacity = 0;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;

	//buffer
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VkDescriptorBufferInfo m_bufferInfo{};
	VkBufferUsageFlags m_bufferUsageFlags = 0;

	//image
	VkImage m_image = VK_NULL_HANDLE;
	VkDescriptorImageInfo m_imageInfo{};
	VkImageUsageFlags m_imageUsageFlags = 0;
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
	void resetData(VkDeviceSize _size = 0, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) override;
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
	void resetData(VkDeviceSize _size = 0, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) override;
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
	enum class IMAGEVALUETYPE
	{
		UINT8,
		INT8,
		UINT32,
		INT32,
		FLOAT32
	};

	VKBufferImage() = delete;
	VKBufferImage(uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step,
		IMAGEVALUETYPE _imageValueType, BUFFERTYPE _type);
	VKBufferImage(VkImage _image, VkImageView _imageView,
		uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step,
		IMAGEVALUETYPE _imageValueType, BUFFERTYPE _type); //for tmp
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
	void resetData(VkDeviceSize _size = 0, VkQueue _queue = VK_NULL_HANDLE, VkFence _fence = nullptr) override;
	void releaseBufferCopy() override;
	void destroy(bool _bf = true, bool _mem = true) override;
protected:
	static VkFormat getFormat(uint32_t _c, IMAGEVALUETYPE _imageValueType, BUFFERTYPE _type)
	{
		if (_type == BUFFERTYPE::DEPTH)
			//return VK_FORMAT_D32_SFLOAT_S8_UINT; //深度+模板
			return VK_FORMAT_D32_SFLOAT; //深度
		switch (_imageValueType) {
		case IMAGEVALUETYPE::UINT8:
			switch (_c) {
			case 1: return VK_FORMAT_R8_UNORM;
			case 2: return VK_FORMAT_R8G8_UNORM;
			case 3: return VK_FORMAT_R8G8B8_UNORM;
			case 4: return VK_FORMAT_R8G8B8A8_UNORM;
			default: break;
			}
			break;
		case IMAGEVALUETYPE::INT8:
			switch (_c) {
			case 1: return VK_FORMAT_R8_SNORM;
			case 2: return VK_FORMAT_R8G8_SNORM;
			case 3: return VK_FORMAT_R8G8B8_SNORM;
			case 4: return VK_FORMAT_R8G8B8A8_SNORM;
			default: break;
			}
			break;
		case IMAGEVALUETYPE::UINT32:
			switch (_c) {
			case 1: return VK_FORMAT_R32_UINT;
			case 2: return VK_FORMAT_R32G32_UINT;
			case 3: return VK_FORMAT_R32G32B32_UINT;
			case 4: return VK_FORMAT_R32G32B32A32_UINT;
			default: break;
			}
			break;
		case IMAGEVALUETYPE::INT32:
			switch (_c) {
			case 1: return VK_FORMAT_R32_SINT;
			case 2: return VK_FORMAT_R32G32_SINT;
			case 3: return VK_FORMAT_R32G32B32_SINT;
			case 4: return VK_FORMAT_R32G32B32A32_SINT;
			default: break;
			}
			break;
		case IMAGEVALUETYPE::FLOAT32:
			switch (_c) {
			case 1: return VK_FORMAT_R32_SFLOAT;
			case 2: return VK_FORMAT_R32G32_SFLOAT;
			case 3: return VK_FORMAT_R32G32B32_SFLOAT;
			case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			default: break;
			}
			break;
		default:
			return VK_FORMAT_UNDEFINED;
		}
		return VK_FORMAT_UNDEFINED;
	}
	void create(uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step,
		IMAGEVALUETYPE _imageValueType, BUFFERTYPE _type);
	void create(VkImage _image, VkImageView _imageView,
		uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step,
		IMAGEVALUETYPE _imageValueType, BUFFERTYPE _type); //for tmp

protected:
	std::shared_ptr<VKBufferTrans> m_bufferCopy = nullptr;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_channels = 0;
	uint32_t m_step = 1;
	VkFormat m_format = VK_FORMAT_UNDEFINED;
	VkImageSubresourceRange m_subResourceRange{};
};

class VULKAN_LYJ_API VKBufferColorImage : public VKBufferImage
{
public:
	VKBufferColorImage(uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step, IMAGEVALUETYPE _imageValueType);
	VKBufferColorImage(VkImage _image, VkImageView _imageView,
		uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step,
		IMAGEVALUETYPE _imageValueType);
	~VKBufferColorImage();
};

class VULKAN_LYJ_API VKBufferSamplerImage : public VKBufferImage
{
public:
	VKBufferSamplerImage(uint32_t _w, uint32_t _h, uint32_t _c = 4, uint32_t _step = 1, IMAGEVALUETYPE _imageValueType = IMAGEVALUETYPE::UINT8);
	~VKBufferSamplerImage();
};

class VULKAN_LYJ_API VKBufferDepthImage : public VKBufferImage
{
public:
	VKBufferDepthImage(uint32_t _w, uint32_t _h);
	~VKBufferDepthImage();
};



NSP_VULKAN_LYJ_END


#endif // !VULKAN_BUFFER_H

