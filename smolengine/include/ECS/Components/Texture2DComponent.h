#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Primitives/Texture.h"

#include <string>
#include <glm/glm.hpp>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct Texture2DComponent: public BaseComponent
	{
		Texture2DComponent() = default;
		Texture2DComponent(uint32_t id)
			:BaseComponent(id) {}

		void                    Load(const std::string& path);
		const Ref<Texture>&     GetTexture() const;

	public:
		bool                    Enabled = true;
		int                     LayerIndex = 0;
		glm::vec4               Color = glm::vec4(1.0f);

	private:
		Ref<Texture>            Texture = nullptr;
		std::string             TexturePath = "";

	private:

		friend class cereal::access;
		friend class EditorLayer;
		friend class Scene;

		template<typename Archive>
		void serialize(Archive & archive)
		{
			archive(Color.r, Color.g, Color.b, Color.a, LayerIndex, TexturePath,  Enabled, ComponentID);
		}
	};
}