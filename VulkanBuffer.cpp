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
	//destroy();
}
void VKBufferAbr::resize(VkDeviceSize _size)
{
	if (_size <= m_size)
		return;
	else {
		m_size = _size;
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
void VKBufferAbr::destroy(bool _bf, bool _mem)
{
	if (m_buffer && _bf) {
		vkDestroyBuffer(m_device, m_buffer, nullptr);
		m_size = 0;
	}
	if (m_memory && _mem) {
		vkFreeMemory(m_device, m_memory, nullptr);
		m_capacity = 0;
	}
}
VkResult VKBufferAbr::createVkBuffer()
{
	destroy(true, false);
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.usage = m_usageFlags;
	bufferCreateInfo.size = m_size;
	//VK_SHARING_MODE_EXCLUSIVE = 0, 资源被队列访问时独占
	//VK_SHARING_MODE_CONCURRENT = 1, 允许多个队列访问
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
VkResult VKBufferAbr::flush(VkDeviceSize size, VkDeviceSize offset) {
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkFlushMappedMemoryRanges(m_device, 1, &mappedRange);
}
VkResult VKBufferAbr::invalidate(VkDeviceSize size, VkDeviceSize offset) {
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkInvalidateMappedMemoryRanges(m_device, 1, &mappedRange);
}
void VKBufferAbr::setupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
	m_bufferInfo.offset = offset;
	m_bufferInfo.buffer = m_buffer;
	m_bufferInfo.range = size;
}




VKBufferTrans::VKBufferTrans()
{
	m_usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	m_memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	m_type = BUFFERTYPE::TRANSFER;
}
VKBufferTrans::~VKBufferTrans()
{}
void VKBufferTrans::upload(VkDeviceSize _size, void* _data, VkQueue _queue, VkFence _fence)
{
	resize(_size);
	mapGPU2CPU(_size, 0);
	copyTo(_data, _size, true);
	flush(_size);
	unmapGPU2CPU();
}
void VKBufferTrans::download(VkDeviceSize _size, void* _data, VkQueue _queue, VkFence _fence)
{
	resize(_size);
	mapGPU2CPU(_size, 0);
	invalidate(_size);
	copyTo(_data, _size, false);
	unmapGPU2CPU();
}
VkResult VKBufferTrans::mapGPU2CPU(VkDeviceSize _size, VkDeviceSize _offset) {
	return vkMapMemory(m_device, m_memory, _offset, _size, 0, &m_mapped);
}
void VKBufferTrans::copyTo(void* _data, VkDeviceSize _size, bool _isFromCPU) {
	assert(m_mapped);
	if(_isFromCPU)
		memcpy(m_mapped, _data, _size);
	else
		memcpy(_data, m_mapped, _size);
}
void VKBufferTrans::unmapGPU2CPU() {
	if (m_mapped) {
		vkUnmapMemory(m_device, m_memory);
		m_mapped = nullptr;
	}
}




VKBufferDevice::VKBufferDevice()
{
	m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
}
VKBufferDevice::~VKBufferDevice()
{}
void VKBufferDevice::upload(VkDeviceSize _size, void* _data, VkQueue _queue, VkFence _fence)
{
	if (_queue == VK_NULL_HANDLE) {
		std::cout << "need queue!" << std::endl;
		return;
	}
	resize(_size);
	if (m_bufferCopy == nullptr)
		m_bufferCopy.reset(new VKBufferTrans());
	m_bufferCopy->upload(_size, _data);

	LYJ_VK::VKCommandBufferBarrier cmdBufferBarrierSrc(
		{ m_bufferCopy->getBuffer() },
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	LYJ_VK::VKCommandBufferBarrier cmdBufferBarrierDst(
		{ m_buffer },
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	LYJ_VK::VKCommandTransfer cmdTransfer(m_bufferCopy->getBuffer(), m_buffer, m_bufferCopy->getSize());
	LYJ_VK::VKImp vkImp(0);
	vkImp.setCmds({ &cmdBufferBarrierSrc, &cmdTransfer, &cmdBufferBarrierDst });
	vkImp.run(_queue, _fence);

	//auto cmdPool = GetLYJVKInstance()->m_computeCommandPool;

	//VkCommandBufferAllocateInfo cmdAllocInfo{};
	//cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	//cmdAllocInfo.commandPool = cmdPool;
	//cmdAllocInfo.commandBufferCount = 1;
	//cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	//VkCommandBuffer vkCmdBuffer;
	//VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &vkCmdBuffer));
	//VkCommandBufferBeginInfo cmdBeginInfo{};
	//cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//VK_CHECK_RESULT(vkBeginCommandBuffer(vkCmdBuffer, &cmdBeginInfo));

	//cmdBufferBarrierSrc.record(vkCmdBuffer);
	//VkBufferCopy bufferCopy{};
	//bufferCopy.size = m_size;
	//vkCmdCopyBuffer(vkCmdBuffer, m_bufferCopy->getBuffer(), m_buffer, 1, &bufferCopy);
	//cmdBufferBarrierDst.record(vkCmdBuffer);

	//VK_CHECK_RESULT(vkEndCommandBuffer(vkCmdBuffer));
	//
	//VkSubmitInfo submitInfo{};
	//submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//submitInfo.commandBufferCount = 1;
	//submitInfo.pCommandBuffers = &vkCmdBuffer;
	//VK_CHECK_RESULT(vkQueueSubmit(_queue, 1, &submitInfo, nullptr));

	//vkFreeCommandBuffers(m_device, cmdPool, 1, &vkCmdBuffer);
}
void VKBufferDevice::download(VkDeviceSize _size, void* _data, VkQueue _queue, VkFence _fence)
{
	if (_queue == VK_NULL_HANDLE) {
		std::cout << "need queue!" << std::endl;
		return;
	}
	LYJ_VK::VKCommandMemoryBarrier cmdMemoryBarrier(
		VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	LYJ_VK::VKCommandTransfer cmdTransfer(m_buffer, m_bufferCopy->getBuffer(), m_size);
	LYJ_VK::VKImp vkImp(0);
	vkImp.setCmds({ &cmdMemoryBarrier, &cmdTransfer });
	VKFence fence;
	vkImp.run(_queue, fence.ptr());
	fence.wait();
	if (m_bufferCopy == nullptr)
		m_bufferCopy.reset(new VKBufferTrans());
	m_bufferCopy->download(_size, _data);
}


VKBufferCompute::VKBufferCompute()
{
	m_usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	m_type = BUFFERTYPE::COMPUTE;
}
VKBufferCompute::~VKBufferCompute()
{}

VKBufferUniform::VKBufferUniform()
{
	m_usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	m_memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	m_type = BUFFERTYPE::UNIFORM;
}
VKBufferUniform::~VKBufferUniform()
{}

VKBufferVertex::VKBufferVertex()
{
	m_usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	m_type = BUFFERTYPE::VERTEX;
}
VKBufferVertex::~VKBufferVertex()
{}

VKBufferIndex::VKBufferIndex()
{
	m_usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	m_type = BUFFERTYPE::INDEX;
}
VKBufferIndex::~VKBufferIndex()
{}



NSP_VULKAN_LYJ_END


