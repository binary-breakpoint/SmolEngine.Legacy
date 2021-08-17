#pragma once
#include "Primitives/BufferLayout.h"

#include <glm/glm/glm.hpp>
#include <memory>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	extern "C++"
	{
		template<typename T>
		using Scope = std::unique_ptr<T>;

		template<typename T>
		using Ref = std::shared_ptr<T>;
	}

	struct VertexInputInfo
	{
		VertexInputInfo() = default;
		VertexInputInfo(uint32_t stride, const BufferLayout& layout, bool isInputRateInstance = false)
			:
			Stride(stride),
			Layout(layout),
			IsInputRateInstance(isInputRateInstance) {}

		uint32_t          Stride = 0;
		BufferLayout      Layout{};
		bool              IsInputRateInstance = false;
	};

	struct PBRVertex
	{
		glm::vec3          Pos = glm::vec3(0.0f);
		glm::vec3          Normals = glm::vec3(0.0f);
		glm::vec3          Tangent = glm::vec3(0.0f);
		glm::vec2          UVs = glm::vec2(0.0f);
		glm::ivec4         jointIndices = glm::ivec4(0);
		glm::vec4          jointWeight = glm::vec4(0.0f);
	};

	struct TextVertex
	{
		glm::vec3 Pos;
		glm::vec2 UV;
	};

	struct BoundingBox
	{
		glm::vec3 min = glm::vec3(0);
		glm::vec3 max = glm::vec3(0);

		void SetBoundingBox(const glm::vec3& min, const glm::vec3& max);
		void CalculateAABB(const glm::mat4& m);
	};

	struct SceneViewProjection;
	struct RendererStorageBase
	{
		virtual void  Initilize() = 0;
		virtual void  BeginSubmit(SceneViewProjection* sceneViewProj) = 0;
		virtual void  EndSubmit() = 0;
		virtual void  OnResize(uint32_t width, uint32_t height) {};
	};

	enum class ImageFilter: int
	{
		NEAREST,
		LINEAR,
	};

	struct GLSL_BOOLPAD
	{
		bool data[3];
	};
}