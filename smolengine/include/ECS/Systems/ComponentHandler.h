#pragma once
#include "Core/Core.h"

#include <string>

namespace SmolEngine
{
	enum class MeshTypeEX : uint32_t;

	struct MeshComponent;
	struct Texture2DComponent;
	struct Rigidbody2DComponent;
	struct RigidbodyComponent;
	struct StaticbodyComponent;
	struct ScriptComponent;
	struct AudioSourceComponent;
	struct AnimationControllerComponent;

	class Actor;
	class Mesh;

	class ComponentHandler
	{
	public:

		static bool ValidateMeshComponent(MeshComponent* comp, const std::string& filePath, bool pooling = true);
		static bool ValidateMeshComponent(MeshComponent* comp, MeshTypeEX type);
		static bool SetMeshMaterial(MeshComponent* comp, uint32_t mesh_index, const std::string& material_path);
		static bool AddAnimationControllerClip(AnimationControllerComponent* comp, const std::string& path);
		static bool ValidateAudioSourceComponent(AudioSourceComponent* comp, const std::string& clip_path);
		static bool ValidateTexture2DComponent(Texture2DComponent* comp, const std::string& filePath);
		static bool ValidateBody2DComponent(Rigidbody2DComponent* comp, Ref<Actor>& actor);
		static bool ValidateRigidBodyComponent(RigidbodyComponent* comp, Ref<Actor>& actor);
	};
}