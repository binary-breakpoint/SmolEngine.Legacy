#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Primitives/Mesh.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	enum class MeshTypeEX : uint32_t
	{
		Custom,
		Cube,
		Sphere,
		Capsule,
		Torus
	};

	struct MeshComponent: public BaseComponent
	{
		MeshComponent() = default;
		MeshComponent(uint32_t id)
			:BaseComponent(id) {}

		bool           bIsStatic = false;
		bool           bShow = true;			
		MeshTypeEX     eType = MeshTypeEX::Custom;
		Ref<Mesh>      Mesh = nullptr;
		Ref<MeshView>  View = nullptr;
		std::string    FilePath = "";			

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(eType, bIsStatic, bShow, FilePath, ComponentID);
		}
	};
}