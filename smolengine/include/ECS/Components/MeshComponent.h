#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Animation/AnimationController.h"
#include "Materials/Material3D.h"
#include "Materials/PBRFactory.h"
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

		void                      LoadMesh(MeshTypeEX type);
		void                      LoadMesh(const std::string& path);
		void                      LoadPBRHandle(const std::string& path, uint32_t nodeIndex = 0);
		void                      LoadAnimation(const std::string& path);
		void                      SetMaterial(const Ref<Material3D>& material, uint32_t nodeIndex = 0);
		void                      SetAnimationController(const Ref<AnimationController>& contoller);
		const Ref<Mesh>&          GetMesh() const;
		const Ref<MeshView>&      GetMeshView() const;
		Ref<AnimationController>& GetAnimationController();

	public:
		bool                     bIsStatic = false;
		bool                     bShow = true;	

	private:
		MeshTypeEX               eType = MeshTypeEX::Custom;
		Ref<AnimationController> AnimContoller = nullptr;
		Ref<Mesh>                Mesh = nullptr;
		Ref<MeshView>            View = nullptr;
		std::string              FilePath = "";		
		std::vector<std::string> AnimPaths;

	private:
		friend class cereal::access;
		friend class Scene;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(eType, bIsStatic, bShow, FilePath, View, AnimPaths, ComponentID);
		}
	};
}