#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Vulkan/VulkanDescriptor.h"

#include "Common/Texture.h"
#include "Common/CubeMap.h"
#include "GraphicsContext.h"

#include "Vulkan/VulkanShader.h"
#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanBufferPool.h"
#include "Vulkan/VulkanPBR.h"
#include "Vulkan/VulkanTexture.h"


#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	VulkanDescriptor::VulkanDescriptor()
	{
		m_Device = VulkanContext::GetDevice().GetLogicalDevice();
	}

	VulkanDescriptor::~VulkanDescriptor()
	{
		Free();
	}

	void VulkanDescriptor::Free()
	{
		if (m_Device)
		{
			if (m_DescriptorSetLayout != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
			}

			m_LocalBuffers.clear();
		}
	}

	void VulkanDescriptor::GenDescriptorSet(VulkanShader* shader, VkDescriptorPool pool)
	{
		std::vector< VkDescriptorSetLayoutBinding> layouts;
		ReflectionData* refData = shader->m_ReflectionData;
		layouts.reserve(refData->Buffers.size() + refData->Resources.size());

		for (auto& [bindingPoint, buffer] : refData->Buffers)
		{
			VkDescriptorType type = buffer.Type == BufferType::Uniform ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

			VkDescriptorSetLayoutBinding layoutBinding = {};
			{
				layoutBinding.binding = buffer.BindingPoint;
				layoutBinding.descriptorType = type;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = VulkanShader::GetVkShaderStage(buffer.Stage);
			}

			layouts.push_back(layoutBinding);
		}

		// Samplers
		for (auto& info : refData->Resources)
		{
			auto& [bindingPoint, res] = info;
			VkDescriptorSetLayoutBinding layoutBinding = {};
			{
				layoutBinding.binding = res.BindingPoint;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				layoutBinding.descriptorCount  = res.ArraySize > 0 ? res.ArraySize : 1;
				layoutBinding.stageFlags = VulkanShader::GetVkShaderStage(res.Stage);
			}

			layouts.push_back(layoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		{
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(layouts.size());
			layoutInfo.pBindings = layouts.data();

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout));
		}

		VkDescriptorSetAllocateInfo allocateInfo = {};
		{
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = pool;
			allocateInfo.descriptorSetCount = 1;
			allocateInfo.pSetLayouts = &m_DescriptorSetLayout;

			VK_CHECK_RESULT(vkAllocateDescriptorSets(m_Device, &allocateInfo, &m_DescriptorSet));
		}
	}

	// TODO: refactor
	void VulkanDescriptor::GenBuffersDescriptors(VulkanShader* shader)
	{
		ReflectionData* refData = shader->m_ReflectionData;
		for (auto& [key, buffer] : refData->Buffers)
		{
			VkDescriptorBufferInfo descriptorBufferInfo = {};
			ShaderBufferInfo bufferInfo = {};

			VkDescriptorType type = buffer.Type == BufferType::Uniform ?  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			VkBufferUsageFlags usage = buffer.Type == BufferType::Uniform ?  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			size_t size = buffer.Size;

			{
				const auto& it = shader->m_CreateInfo->BufferInfos.find(buffer.BindingPoint);
				if (it == shader->m_CreateInfo->BufferInfos.end())
				{
					if (buffer.Type == BufferType::Storage)
					{
						bool found = false;

						{
							if (bufferInfo.bGlobal)
								found = VulkanBufferPool::GetSingleton()->IsBindingExist(buffer.BindingPoint);
							else
							{
								auto& f_res = m_LocalBuffers.find(buffer.BindingPoint);
								if (f_res != m_LocalBuffers.end())
									found = true;

							}
						}

						if (found == false)
						{
#ifdef FROSTIUM_DEBUG
							NATIVE_ERROR("Storage buffer dataSize must be declared inside GraphicsPipelineShaderCreateInfo!");
#endif
							continue;
						}
					}
				}
				else
				{
					if (buffer.Type == BufferType::Storage)
						size = it->second.Size;

					bufferInfo = it->second;
				}
			}


			if (bufferInfo.bGlobal)
				VulkanBufferPool::GetSingleton()->Add(size, buffer.BindingPoint, usage, descriptorBufferInfo, bufferInfo.bStatic, bufferInfo.Data);
			else
			{
				const auto& it = m_LocalBuffers.find(buffer.BindingPoint);
				if (it == m_LocalBuffers.end())
				{
					Ref<BufferObject> object = std::make_shared<BufferObject>();
					{
						if (bufferInfo.bStatic)
						{
							usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
							object->VkBuffer.CreateStaticBuffer(bufferInfo.Data, size, usage);
						}
						else
						{
							VkMemoryPropertyFlags mem = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

							object->VkBuffer.CreateBuffer(size, mem, usage);
						}

						object->DesriptorBufferInfo.buffer = object->VkBuffer.GetBuffer();
						object->DesriptorBufferInfo.offset = 0;
						object->DesriptorBufferInfo.range = size;
					}

					m_LocalBuffers[buffer.BindingPoint] = object;
					descriptorBufferInfo = object->DesriptorBufferInfo;
				}
				else
				{
					descriptorBufferInfo = it->second->DesriptorBufferInfo;
				}
			}

			m_WriteSets.push_back(CreateWriteSet(m_DescriptorSet,
				buffer.BindingPoint, &descriptorBufferInfo, type));

			vkUpdateDescriptorSets(m_Device, 1, &m_WriteSets.back(), 0, nullptr);

#ifdef FROSTIUM_DEBUG
			NATIVE_WARN("Created " + buffer.ObjectName + " {}: Members Count: {}, Binding Point: {}",
				buffer.Name, buffer.Uniforms.size(), buffer.BindingPoint);
#endif
		}
	}

	void VulkanDescriptor::GenSamplersDescriptors(VulkanShader* shader)
	{
		ReflectionData* refData = shader->m_ReflectionData;
#ifndef FROSTIUM_OPENGL_IMPL
		m_ImageInfo = GraphicsContext::s_Instance->m_DummyTexure->GetVulkanTexture()->m_DescriptorImageInfo;
#endif
		for (auto& [bindingPoint, res] : refData->Resources)
		{
			if (res.Dimension == 3) // cubeMap
			{
				auto& cube = GraphicsContext::GetSingleton()->m_DummyCubeMap;
				VkWriteDescriptorSet writeSet = {};
				{
					writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeSet.dstSet = m_DescriptorSet;
					writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writeSet.dstBinding = res.BindingPoint;
					writeSet.dstArrayElement = 0;
					writeSet.descriptorCount = 1;
					writeSet.pImageInfo = &cube->GetTexture()->GetVulkanTexture()->m_DescriptorImageInfo;
				}

				m_WriteSets.push_back(writeSet);

				auto& kek = m_WriteSets.back();
				vkUpdateDescriptorSets(m_Device, 1, &m_WriteSets.back(), 0, nullptr);
			}
			else
			{
				std::vector<VkDescriptorImageInfo> infos;

				if (res.ArraySize > 0)
				{
					infos.resize(res.ArraySize);
					for (uint32_t i = 0; i < res.ArraySize; ++i)
					{
						infos[i] = m_ImageInfo;
					}
				}
				else
					infos.push_back(m_ImageInfo);

				m_WriteSets.push_back(CreateWriteSet(m_DescriptorSet,
					res.BindingPoint, infos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER));
				vkUpdateDescriptorSets(m_Device, 1, &m_WriteSets.back(), 0, nullptr);
			}
		}
	}

	bool VulkanDescriptor::Update2DSamplers(const std::vector<VulkanTexture*>& textures, uint32_t bindingPoint)
	{
		VkWriteDescriptorSet* writeSet = nullptr;
		for (auto& set: m_WriteSets)
		{
			if (set.dstBinding == bindingPoint)
			{
				writeSet = &set;
				break;
			}
		}

		if (!writeSet)
			return false;

		std::vector<VkDescriptorImageInfo> infos(writeSet->descriptorCount);
		for (uint32_t i = 0; i < writeSet->descriptorCount; ++i)
		{
			if (textures.size() > i)
			{
				if (textures[i])
				{
					infos[i] = textures[i]->m_DescriptorImageInfo;
					continue;
				}
			}

			infos[i] = m_ImageInfo;
		}

		*writeSet = CreateWriteSet(m_DescriptorSet,
			bindingPoint, infos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		vkUpdateDescriptorSets(m_Device, 1, writeSet, 0, nullptr);
		return true;
	}

	bool VulkanDescriptor::UpdateImageResource(uint32_t bindingPoint, const VkDescriptorImageInfo& imageInfo)
	{
		VkWriteDescriptorSet* writeSet = nullptr;
		for (auto& set : m_WriteSets)
		{
			if (set.dstBinding == bindingPoint)
			{
				writeSet = &set;
				break;
			}
		}

		if (!writeSet)
			return false;

		writeSet->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet->dstSet = m_DescriptorSet;
		writeSet->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeSet->dstBinding = bindingPoint;
		writeSet->dstArrayElement = 0;
		writeSet->descriptorCount = 1;
		writeSet->pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_Device, 1, writeSet, 0, nullptr);
		return true;
	}

	bool VulkanDescriptor::UpdateCubeMap(const VulkanTexture* cubeMap, uint32_t bindingPoint)
	{
		if (!cubeMap)
			return false;

		VkWriteDescriptorSet* writeSet = nullptr;
		for (auto& set : m_WriteSets)
		{
			if (set.dstBinding == bindingPoint)
			{
				writeSet = &set;
				break;
			}
		}

		if (!writeSet)
			return false;

		*writeSet = CreateWriteSet(m_DescriptorSet,
			bindingPoint, { cubeMap->m_DescriptorImageInfo }, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		vkUpdateDescriptorSets(m_Device, 1, writeSet, 0, nullptr);
		return true;
	}

	bool VulkanDescriptor::UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset)
	{
		VulkanBuffer* buffer = nullptr;
		// Local Buffers
		{
			if (m_LocalBuffers.size() > 0)
			{
				auto& it = m_LocalBuffers.find(binding);
				if (it != m_LocalBuffers.end())
				{
					buffer = &it->second->VkBuffer;
					buffer->SetData(data, size, offset);
					return true;
				}
			}
		}


		// Global Buffers
		{
			buffer = VulkanBufferPool::GetSingleton()->GetBuffer(binding);
			if (buffer)
			{
				buffer->SetData(data, size, offset);
				return true;
			}
		}

		return false;
	}

	void VulkanDescriptor::UpdateWriteSets()
	{
		vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(m_WriteSets.size()), m_WriteSets.data(), 0, nullptr);
	}

	const VkDescriptorSet VulkanDescriptor::GetDescriptorSets() const
	{
		return m_DescriptorSet;
	}

	VkWriteDescriptorSet VulkanDescriptor::CreateWriteSet(VkDescriptorSet descriptorSet, uint32_t binding, 
		VkDescriptorBufferInfo* descriptorBufferInfo, VkDescriptorType descriptorType)
	{
		const auto& device = *VulkanContext::GetDevice().GetLogicalDevice();

		VkWriteDescriptorSet writeSet = {};
		{
			writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = binding;
			writeSet.dstArrayElement = 0;
			writeSet.descriptorType = descriptorType;
			writeSet.descriptorCount = 1;
			writeSet.pBufferInfo = descriptorBufferInfo;
		}

		return writeSet;
	}

	VkWriteDescriptorSet VulkanDescriptor::CreateWriteSet(VkDescriptorSet descriptorSet, uint32_t binding,
		VkDescriptorImageInfo* imageInfo, VkDescriptorType descriptorType)
	{
		VkWriteDescriptorSet writeSet = {};
		{
			writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeSet.dstSet = descriptorSet;
			writeSet.descriptorType = descriptorType;
			writeSet.descriptorCount = 1;
			writeSet.dstArrayElement = 0;
			writeSet.dstBinding = binding;
			writeSet.pImageInfo = imageInfo;
		}

		return writeSet;
	}

	VkWriteDescriptorSet VulkanDescriptor::CreateWriteSet(VkDescriptorSet descriptorSet, uint32_t binding,
		const std::vector<VkDescriptorImageInfo>& descriptorimageInfos, VkDescriptorType descriptorType)
	{
		VkWriteDescriptorSet writeSet = {};
		{
			writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeSet.dstSet = descriptorSet;
			writeSet.descriptorType = descriptorType;
			writeSet.dstBinding = binding;
			writeSet.dstArrayElement = 0;
			writeSet.descriptorCount = static_cast<uint32_t>(descriptorimageInfos.size());
			writeSet.pImageInfo = descriptorimageInfos.data();
		}

		return writeSet;
	}
}
#endif