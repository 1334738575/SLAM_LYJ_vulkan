#include "VulkanBuffer.h"
#include "VulkanCommon.h"
#include "VulkanImplement.h"
#include "VulkanCommand.h"

NSP_VULKAN_LYJ_BEGIN

VKBufferAbr::VKBufferAbr()
{
	m_device = GetLYJVKInstance()->m_device;
}
VKBufferAbr::~VKBufferAbr()
{
	// destroy();
}
void VKBufferAbr::resize(VkDeviceSize _size)
{
	if (_size <= m_size)
		return;
	else
	{
		m_size = (_size + 63) / 64 * 64;
		if (createVkBuffer() != VK_SUCCESS)
			return destroy();
		if (m_size > m_capacity)
			if (createVkMemory() != VK_SUCCESS)
				return destroy();
	}
	if (vkBindBufferMemory(m_device, m_buffer, m_memory, 0) != VK_SUCCESS)
		return destroy();
	setupDescriptor(_size, 0);
}
VkResult VKBufferAbr::createVkBuffer()
{
	destroy(true, false);
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.usage = m_bufferUsageFlags;
	bufferCreateInfo.size = m_size;
	// VK_SHARING_MODE_EXCLUSIVE = 0, 资源被队列访问时独占
	// VK_SHARING_MODE_CONCURRENT = 1, 允许多个队列访问
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	return vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &m_buffer);
}
VkResult VKBufferAbr::createVkMemory()
{
	VkMemoryRequirements memoryRequires;
	vkGetBufferMemoryRequirements(m_device, m_buffer, &memoryRequires);
	m_capacity = memoryRequires.size;
	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequires.size;
	memoryAllocateInfo.memoryTypeIndex = GetLYJVKInstance()->getMemoryTypeIndex(memoryRequires.memoryTypeBits, m_memoryPropertyFlags);
	return vkAllocateMemory(m_device, &memoryAllocateInfo, nullptr, &m_memory);
}
VkResult VKBufferAbr::flush(VkDeviceSize size, VkDeviceSize offset)
{
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkFlushMappedMemoryRanges(m_device, 1, &mappedRange);
}
VkResult VKBufferAbr::invalidate(VkDeviceSize size, VkDeviceSize offset)
{
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkInvalidateMappedMemoryRanges(m_device, 1, &mappedRange);
}
void VKBufferAbr::setupDescriptor(VkDeviceSize size, VkDeviceSize offset)
{
	m_bufferInfo.offset = offset;
	m_bufferInfo.buffer = m_buffer;
	m_bufferInfo.range = size;
}

VKBufferTrans::VKBufferTrans()
{
	m_bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	m_memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	m_type = BUFFERTYPE::TRANSFER;
}
VKBufferTrans::~VKBufferTrans()
{
}
void VKBufferTrans::upload(VkDeviceSize _size, void *_data, VkQueue _queue, VkFence _fence)
{
	resize(_size);
	mapGPU2CPU(_size, 0);
	copyTo(_data, _size, true);
	flush(_size);
	unmapGPU2CPU();
}
void VKBufferTrans::download(VkDeviceSize _size, void *_data, VkQueue _queue, VkFence _fence)
{
	resize(_size);
	mapGPU2CPU(_size, 0);
	invalidate(_size);
	copyTo(_data, _size, false);
	unmapGPU2CPU();
	return;
}
void *VKBufferTrans::download(VkDeviceSize _size, VkQueue _queue, VkFence _fence)
{
	resize(_size);
	mapGPU2CPU(_size, 0);
	return m_mapped;
}
void VKBufferTrans::resetData(VkDeviceSize _size, VkQueue _queue, VkFence _fence)
{
	if (_size)
		return;
	resize(_size);
	mapGPU2CPU(_size, 0);
	memset(m_mapped, 0, _size);
	flush(_size);
	unmapGPU2CPU();
}
void VKBufferTrans::destroy(bool _bf, bool _mem)
{
	if (m_buffer && _bf)
	{
		vkDestroyBuffer(m_device, m_buffer, nullptr);
		// m_size = 0;
	}
	if (m_memory && _mem)
	{
		vkFreeMemory(m_device, m_memory, nullptr);
		m_capacity = 0;
	}
}
VkResult VKBufferTrans::mapGPU2CPU(VkDeviceSize _size, VkDeviceSize _offset)
{
	return vkMapMemory(m_device, m_memory, _offset, _size, 0, &m_mapped);
}
void VKBufferTrans::copyTo(void *_data, VkDeviceSize _size, bool _isFromCPU)
{
	assert(m_mapped);
	if (_isFromCPU)
		memcpy(m_mapped, _data, _size);
	else
		memcpy(_data, m_mapped, _size);
}
void VKBufferTrans::unmapGPU2CPU()
{
	if (m_mapped)
	{
		vkUnmapMemory(m_device, m_memory);
		m_mapped = nullptr;
	}
}

VKBufferDevice::VKBufferDevice()
{
	m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}
VKBufferDevice::~VKBufferDevice()
{
}
void VKBufferDevice::upload(VkDeviceSize _size, void *_data, VkQueue _queue, VkFence _fence)
{
	if (_queue == VK_NULL_HANDLE)
	{
		std::cout << "need queue!" << std::endl;
		return;
	}
	resize(_size);
	if (m_bufferCopy == nullptr)
		m_bufferCopy.reset(new VKBufferTrans());
	VkDeviceSize sss = (_size + 63) / 64 * 64;
	m_bufferCopy->upload(sss, _data);

	LYJ_VK::VKCommandBufferBarrier cmdBufferBarrierSrc(
		{m_bufferCopy->getBuffer()},
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	LYJ_VK::VKCommandBufferBarrier cmdBufferBarrierDst(
		{m_buffer},
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	LYJ_VK::VKCommandTransfer cmdTransfer(m_bufferCopy->getBuffer(), m_buffer, m_bufferCopy->getSize());
	LYJ_VK::VKImp vkImp(0);
	vkImp.setCmds({&cmdBufferBarrierSrc, &cmdTransfer, &cmdBufferBarrierDst});
	vkImp.run(_queue, _fence);
}
void VKBufferDevice::download(VkDeviceSize _size, void *_data, VkQueue _queue, VkFence _fence)
{
	if (_queue == VK_NULL_HANDLE)
	{
		std::cout << "need queue!" << std::endl;
		return;
	}
	if (m_bufferCopy == nullptr || _data == nullptr)
	{
		return;
	}
	LYJ_VK::VKCommandMemoryBarrier cmdMemoryBarrier;
	LYJ_VK::VKCommandMemoryBarrier cmdMemoryBarrier2;
	LYJ_VK::VKCommandTransfer cmdTransfer(m_buffer, m_bufferCopy->getBuffer(), m_size);
	LYJ_VK::VKImp vkImp(0);
	vkImp.setCmds({&cmdMemoryBarrier, &cmdTransfer, &cmdMemoryBarrier2});
	VKFence fence;
	vkImp.run(_queue, fence.ptr());
	fence.wait();
	m_bufferCopy->download(_size, _data);
	return;
}
void *VKBufferDevice::download(VkDeviceSize _size, VkQueue _queue, VkFence _fence)
{
	if (_queue == VK_NULL_HANDLE)
	{
		std::cout << "need queue!" << std::endl;
		return nullptr;
	}
	m_bufferCopy.reset(new VKBufferTrans());
	void *ret = m_bufferCopy->download(_size);
	LYJ_VK::VKCommandMemoryBarrier cmdMemoryBarrier;
	LYJ_VK::VKCommandMemoryBarrier cmdMemoryBarrier2;
	LYJ_VK::VKCommandTransfer cmdTransfer(m_buffer, m_bufferCopy->getBuffer(), m_size);
	LYJ_VK::VKImp vkImp(0);
	vkImp.setCmds({&cmdMemoryBarrier, &cmdTransfer, &cmdMemoryBarrier2});
	vkImp.run(_queue, _fence);
	return ret;
}
void VKBufferDevice::resetData(VkDeviceSize _size, VkQueue _queue, VkFence _fence)
{
	if (_queue == VK_NULL_HANDLE)
	{
		std::cout << "need queue!" << std::endl;
		return;
	}
	if (_size == 0 && m_size == 0)
		return;
	if(_size != 0)
		resize(_size);
	VkDeviceSize sss = m_size;

	LYJ_VK::VKCommandBufferBarrier cmdBufferBarrierSrc(
		{ m_buffer },
		VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	LYJ_VK::VKCommandBufferBarrier cmdBufferBarrierDst(
		{ m_buffer },
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	LYJ_VK::VKCommandFiller cmdFiller(m_buffer, sss);

	LYJ_VK::VKImp vkImp(0);
	vkImp.setCmds({ &cmdBufferBarrierSrc, &cmdFiller, &cmdBufferBarrierDst });
	vkImp.run(_queue, _fence);
}
void VKBufferDevice::releaseBufferCopy()
{
	if (m_bufferCopy)
		m_bufferCopy->destroy();
	m_bufferCopy.reset();
}
void VKBufferDevice::destroy(bool _bf, bool _mem)
{
	if (m_bufferCopy)
		m_bufferCopy->destroy();
	if (m_buffer && _bf)
	{
		vkDestroyBuffer(m_device, m_buffer, nullptr);
		// m_size = 0;
	}
	if (m_memory && _mem)
	{
		vkFreeMemory(m_device, m_memory, nullptr);
		m_capacity = 0;
	}
}

VKBufferCompute::VKBufferCompute()
{
	m_bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	m_type = BUFFERTYPE::COMPUTE;
}
VKBufferCompute::~VKBufferCompute()
{
}

VKBufferUniform::VKBufferUniform()
{
	m_bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	m_type = BUFFERTYPE::UNIFORM;
}
VKBufferUniform::~VKBufferUniform()
{
}

VKBufferVertex::VKBufferVertex()
{
	m_bufferUsageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	m_type = BUFFERTYPE::VERTEX;
}
VKBufferVertex::~VKBufferVertex()
{
}

VKBufferIndex::VKBufferIndex()
{
	m_bufferUsageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	m_type = BUFFERTYPE::INDEX;
}
VKBufferIndex::~VKBufferIndex()
{
}

VKBufferImage::VKBufferImage(uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step, IMAGEVALUETYPE _imageValueType, BUFFERTYPE _type)
{
	create(_w, _h, _c, _step, _imageValueType, _type);
}
VKBufferImage::VKBufferImage(VkImage _image, VkImageView _imageView, uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step, IMAGEVALUETYPE _imageValueType, BUFFERTYPE _type)
{
	create(_image, _imageView, _w, _h, _c, _step, _imageValueType, _type);
}
VKBufferImage::~VKBufferImage()
{
}
void VKBufferImage::upload(VkDeviceSize _size, void *_data, VkQueue _queue, VkFence _fence)
{
	if (_queue == VK_NULL_HANDLE)
	{
		std::cout << "need queue!" << std::endl;
		return;
	}
	if (_data == nullptr)
	{
		std::cout << "need data!" << std::endl;
		return;
	}
	if (m_bufferCopy == nullptr)
		m_bufferCopy.reset(new VKBufferTrans());
	m_bufferCopy->upload(_size, _data);

	LYJ_VK::VKCommandImageBarrier cmdImageBarrier1(
		{m_image},
		{m_subResourceRange},
		0, VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	LYJ_VK::VKCommandImageBarrier cmdImageBarrier2(
		{m_image},
		{m_subResourceRange},
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	LYJ_VK::VKCommandTransfer cmdTransfer(m_bufferCopy->getBuffer(), m_image,
										  {m_width, m_height, 1}, m_subResourceRange);
	LYJ_VK::VKImp vkImp(0);
	vkImp.setCmds({&cmdImageBarrier1, &cmdTransfer, &cmdImageBarrier2});
	vkImp.run(_queue, _fence);
}
void VKBufferImage::download(VkDeviceSize _size, void *_data, VkQueue _queue, VkFence _fence)
{
}
void *VKBufferImage::download(VkDeviceSize _size, VkQueue _queue, VkFence _fence)
{
	if (_queue == VK_NULL_HANDLE)
	{
		std::cout << "need queue!" << std::endl;
		return nullptr;
	}
	m_bufferCopy.reset(new VKBufferTrans());
	void *ret = m_bufferCopy->download(_size);

	VkImageLayout imgLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	if (m_type == BUFFERTYPE::DEPTH)
		imgLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	LYJ_VK::VKCommandImageBarrier cmdImageBarrier1(
		{m_image},
		{m_subResourceRange},
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		imgLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	LYJ_VK::VKCommandBufferBarrier cmdBarrier2(
		{m_bufferCopy->getBuffer()},
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT);
	LYJ_VK::VKCommandImageBarrier cmdImageBarrier2(
		{m_image},
		{m_subResourceRange},
		VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imgLayout,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
	LYJ_VK::VKCommandTransfer cmdTransfer(m_image, m_bufferCopy->getBuffer(),
										  {m_width, m_height, 1}, m_subResourceRange);

	LYJ_VK::VKImp vkImp(0);
	vkImp.setCmds({&cmdImageBarrier1, &cmdTransfer, &cmdBarrier2, &cmdImageBarrier2});
	vkImp.run(_queue, _fence);
	return ret;
}
void VKBufferImage::resetData(VkDeviceSize _size, VkQueue _queue, VkFence _fence)
{
	return;
}
void VKBufferImage::releaseBufferCopy()
{
	if (m_bufferCopy)
		m_bufferCopy->destroy();
	m_bufferCopy.reset();
}
void VKBufferImage::destroy(bool _bf, bool _mem)
{
	if (m_bufferCopy)
		m_bufferCopy->destroy();
	if (m_image && _bf)
	{
		vkDestroyImageView(m_device, m_imageInfo.imageView, nullptr);
		vkDestroySampler(m_device, m_imageInfo.sampler, nullptr);
		vkDestroyImage(m_device, m_image, nullptr);
		// m_size = 0;
	}
	if (m_memory && _mem)
	{
		vkFreeMemory(m_device, m_memory, nullptr);
		m_capacity = 0;
	}
}
void VKBufferImage::create(uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step,
						   IMAGEVALUETYPE _imageValueType, BUFFERTYPE _type)
{
	m_width = _w;
	m_height = _h;
	m_channels = _c;
	m_step = _step;
	m_format = getFormat(_c, _imageValueType, _type);
	m_type = _type;
	if (m_type == BUFFERTYPE::COLOR)
		m_imageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	else if (m_type == BUFFERTYPE::SAMPLER)
		m_imageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	else if (m_type == BUFFERTYPE::DEPTH)
		m_imageUsageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_size = m_width * m_height * m_channels * m_step;
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = m_format; // VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.mipLevels = 1;	 // 纹理分辨率层级，依次减半
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	// imageInfo.tiling = VK_IMAGE_TILING_LINEAR; //cpu传输, no use, use buffer to trans
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // 渲染，深度，颜色附件
	imageInfo.usage = m_imageUsageFlags;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.extent.width = m_width;
	imageInfo.extent.height = m_height;
	imageInfo.extent.depth = 1;
	VK_CHECK_RESULT(vkCreateImage(m_device, &imageInfo, nullptr, &m_image));
	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = GetLYJVKInstance()->getMemoryTypeIndex(memRequirements.memoryTypeBits, m_memoryPropertyFlags);
	VK_CHECK_RESULT(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory));
	VK_CHECK_RESULT(vkBindImageMemory(m_device, m_image, m_memory, 0));

	if (m_type == BUFFERTYPE::COLOR || m_type == BUFFERTYPE::SAMPLER)
		m_subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	else if (m_type == BUFFERTYPE::DEPTH)
		m_subResourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	m_subResourceRange.baseMipLevel = 0;
	m_subResourceRange.baseArrayLayer = 0;
	m_subResourceRange.levelCount = 1;
	m_subResourceRange.layerCount = 1;

	m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (m_type == BUFFERTYPE::COLOR || m_type == BUFFERTYPE::SAMPLER)
	{
		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.maxAnisotropy = 1.f;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.mipLodBias = 0.f;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.f;
		samplerCreateInfo.maxLod = 0.f;
		samplerCreateInfo.maxAnisotropy = 1.0;
		samplerCreateInfo.anisotropyEnable = VK_FALSE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(m_device, &samplerCreateInfo, nullptr, &m_imageInfo.sampler));
	}

	VkImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = m_format;
	viewCreateInfo.subresourceRange = m_subResourceRange;
	viewCreateInfo.image = m_image;
	VK_CHECK_RESULT(vkCreateImageView(m_device, &viewCreateInfo, nullptr, &m_imageInfo.imageView));
}
void VKBufferImage::create(VkImage _image, VkImageView _imageView, uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step, IMAGEVALUETYPE _imageValueType, BUFFERTYPE _type)
{
	m_width = _w;
	m_height = _h;
	m_channels = _c;
	m_step = _step;
	m_format = getFormat(_c, _imageValueType, _type);
	m_image = _image;
	m_imageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	m_type = _type;
	m_size = m_width * m_height * m_channels;
	m_subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	m_subResourceRange.baseMipLevel = 0;
	m_subResourceRange.baseArrayLayer = 0;
	m_subResourceRange.levelCount = 1;
	m_subResourceRange.layerCount = 1;
	m_imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_imageInfo.imageView = _imageView;
}

VKBufferColorImage::VKBufferColorImage(uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step, IMAGEVALUETYPE _imageValueType)
	: VKBufferImage(_w, _h, _c, _step, _imageValueType, BUFFERTYPE::COLOR)
{
}
VKBufferColorImage::VKBufferColorImage(VkImage _image, VkImageView _imageView, uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step, IMAGEVALUETYPE _imageValueType)
	: VKBufferImage(_image, _imageView, _w, _h, _c, _step, _imageValueType, BUFFERTYPE::SAMPLER)
{
}
VKBufferColorImage::~VKBufferColorImage()
{
}

VKBufferSamplerImage::VKBufferSamplerImage(uint32_t _w, uint32_t _h, uint32_t _c, uint32_t _step, IMAGEVALUETYPE _imageValueType)
	: VKBufferImage(_w, _h, _c, _step, _imageValueType, BUFFERTYPE::SAMPLER)
{
}
VKBufferSamplerImage::~VKBufferSamplerImage()
{
}

VKBufferDepthImage::VKBufferDepthImage(uint32_t _w, uint32_t _h)
	: VKBufferImage(_w, _h, 1, 4, IMAGEVALUETYPE::FLOAT32, BUFFERTYPE::DEPTH)
{
}
VKBufferDepthImage::~VKBufferDepthImage()
{
}

NSP_VULKAN_LYJ_END
