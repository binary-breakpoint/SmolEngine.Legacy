#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanBuffer.h"

namespace SmolEngine
{
	struct RaytracingPipelineCreateInfo;

	class VulkanACStructure
	{
	public:
		struct Structure
		{
			uint64_t m_DeviceAddress = 0;
			VkAccelerationStructureKHR m_Handle = nullptr;
			VulkanBuffer m_Buffer{};
		};

		void         Build(RaytracingPipelineCreateInfo* info);
		Structure&   GetTopLevel();
		Structure&   GetBottomLevel();

	private:
		void         BuildBottomLevel(RaytracingPipelineCreateInfo* info);
		void         BuildTopLevel(RaytracingPipelineCreateInfo* info);

	private:
		Structure    m_TopLevelAS{};
		Structure    m_BottomLevelAS{};
		VulkanBuffer m_TransformBuffer{};
		VulkanBuffer m_InstancesBuffer{};
	};
}

#endif