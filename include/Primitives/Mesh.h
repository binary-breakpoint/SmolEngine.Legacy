#pragma once
#include "Common/Memory.h"
#include "Common/BoundingBox.h"

#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"
#include "Primitives/PrimitiveBase.h"

#include <memory>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct Primitive;
	struct PBRHandle;

	class Mesh;
	class Material3D;
	class AnimationController;

	struct MeshView
	{
		struct Element
		{
			std::string     m_MaterialPath = "";
			Ref<Material3D> m_Material = nullptr;
			Ref<PBRHandle>  m_PBRHandle = nullptr;

			template<typename Archive>
			void serialize(Archive& archive)
			{
				archive(m_MaterialPath);
			}
		};

		bool                       Serialize(const std::string& path);
		bool                       Deserialize(const std::string& path);
		void                       SetAnimationController(const Ref<AnimationController>& contoller);
		void                       SetPBRHandle(const Ref<PBRHandle>& handle, uint32_t nodeIndex = 0);
		void                       SetMaterial(const Ref<Material3D>& material, uint32_t nodeIndex = 0);
		void                       SetTransform(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0), const glm::vec3& scale = glm::vec3(1));
		const glm::mat4&           GetTransform() const;
		Ref<AnimationController>   GetAnimationController() const;
		Ref<PBRHandle>             GetPBRHandle(uint32_t nodeIndex = 0) const;
		Ref<Material3D>            GetMaterial(uint32_t nodeIndex = 0) const;

	private:
		std::string                m_Path = "";
		glm::mat4                  m_ModelMatrix{};
		std::vector<Element>       m_Elements;
		Ref<AnimationController>   m_AnimationController = nullptr;

		friend class Mesh;
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(m_Path, m_Elements);
		}

	};

	class Mesh: public PrimitiveBase
	{
	public:	
		void                     Free() override;
		bool                     IsGood() const override;
		bool                     LoadFromFile(const std::string& path);
		bool                     LoadFromSave(Ref<MeshView>& view);
								 
		std::vector<Ref<Mesh>>&  GetScene();
		std::vector<Ref<Mesh>>&  GetChilds();
		BoundingBox&             GetAABB();
		uint32_t                 GetChildCount() const;
		size_t                   GetID() const;
		uint32_t                 GetNodeIndex() const;
		std::string              GetName() const;
		Ref<MeshView>            CreateMeshView() const;
		Ref<VertexBuffer>        GetVertexBuffer();
		Ref<IndexBuffer>         GetIndexBuffer();
		Ref<Mesh>                GetMeshByName(const std::string& name);
		Ref<Mesh>                GetMeshByIndex(uint32_t index);  
		bool                     IsRootNode() const;
		static Ref<Mesh>         Create();
							     
	private:				     
		bool                     Build(Ref<Mesh>& mesh, Ref<Mesh> parent, Primitive* primitive);

	private:
		Ref<VertexBuffer>         m_VertexBuffer = nullptr;
		Ref<IndexBuffer>          m_IndexBuffer = nullptr;
		Ref<Mesh>                 m_Root = nullptr;
		Ref<MeshView>             m_DefaultView = nullptr;
		std::string               m_Name = "";
		uint32_t                  m_Index = 0;
		size_t                    m_ID = 0;
		BoundingBox               m_AABB{};
		BoundingBox               m_SceneAABB{};
		std::vector<Ref<Mesh>>    m_Childs;
		std::vector<Ref<Mesh>>    m_Scene;

		friend struct RendererStorage;
		friend struct RendererDrawList;
		friend class GraphicsPipeline;
		friend class MeshPool;
		friend class Animator;
	};
}