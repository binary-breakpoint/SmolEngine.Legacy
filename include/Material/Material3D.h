#pragma once
#include "Material/Material.h"
#include "Common/Vertex.h"

#include <functional>

namespace SmolEngine
{
	struct CommandBuffer;

	class Material3D : public Material
	{
	public:
		Material3D() = default;
		virtual ~Material3D() = default;

		void            SetDrawCallback(const std::function<void(CommandBuffer*, Material3D*)>& callback);
		VertexInputInfo GetVertexInputInfo() const;

	private:
		std::function<void(CommandBuffer*, Material3D*)> m_DrawCallback = nullptr;

		friend class RendererDeferred;
	};
}