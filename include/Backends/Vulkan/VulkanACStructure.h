#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanBuffer.h"
#include "Common/Vertex.h"

namespace SmolEngine
{
	struct RaytracingPipelineCreateInfo;
	struct RaytracingPipelineSceneInfo;

	class Mesh;
	class MeshView;

	class VulkanACStructure
	{
	public:
		struct Structure
		{
			uint64_t m_DeviceAddress = 0;
			VkAccelerationStructureKHR m_Handle = nullptr;
			VulkanBuffer m_Buffer{};
		};

		void         Free();
		void         Build(RaytracingPipelineCreateInfo* info);
		void         BuildScene(RaytracingPipelineSceneInfo* info, bool isStatic);
		Structure&   GetTopLevel();
		Structure&   GetBottomLevel();

	private:
		void         BuildBottomLevel(RaytracingPipelineCreateInfo* info);
		void         BuildTopLevel(RaytracingPipelineCreateInfo* info);

		uint64_t     AddTriangleGeometry(Ref< Mesh>& mesh, Ref<MeshView>& view, const VertexInputInfo& vertexInfo, uint32_t transform_offset = 0);
		void         UpdateTriangleGeometry(uint64_t UUID, Ref<Mesh>& mesh, Ref<MeshView>& view, const VertexInputInfo& vertexInfo, uint32_t transform_offset = 0);

	private:
		struct GeometryInfo
		{
			VkAccelerationStructureGeometryKHR geometry{};
			uint32_t primitive_count{};
			uint32_t transform_offset{};
			bool updated = false;
		};

		Structure m_TopLevelAS{};
		Structure m_BottomLevelAS{};
		VulkanBuffer m_TransformBuffer{};
		VulkanBuffer m_InstancesBuffer{};
		std::map<uint64_t, GeometryInfo> m_Geometries{};
	};
}

#endif