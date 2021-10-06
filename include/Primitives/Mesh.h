#pragma once
#include "Common/Common.h"

#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"
#include "Primitives/PrimitiveBase.h"

#include <memory>

namespace SmolEngine
{
	struct Primitive;

	class Mesh: public PrimitiveBase
	{
	public:	
		void                    Free() override;
		bool                    IsGood() const override;
		bool                    LoadFromFile(const std::string& path, bool pooling = true);

		std::vector<Ref<Mesh>>& GetScene();
		std::vector<Ref<Mesh>>& GetChilds();
		BoundingBox&            GetAABB();
		uint32_t                GetChildCount() const;
		std::string             GetName() const;
		Ref<VertexBuffer>       GetVertexBuffer();
		Ref<IndexBuffer>        GetIndexBuffer();
		Ref<Mesh>               GetMeshByName(const std::string& name);
		Ref<Mesh>               GetMeshByIndex(uint32_t index);  
		bool                    IsRootNode() const;		    
		static Ref<Mesh>        Create();
							    
	private:				    
		bool                    Build(Ref<Mesh>& mesh, Ref<Mesh> parent, Primitive* primitive);

	private:
		std::string             m_Name = "";
		Ref<Mesh>               m_Root = nullptr;
		Ref<VertexBuffer>       m_VertexBuffer = nullptr;
		Ref<IndexBuffer>        m_IndexBuffer = nullptr;
		BoundingBox             m_AABB = {};
		std::vector<Ref<Mesh>>  m_Childs;
		std::vector<Ref<Mesh>>  m_Scene;

		friend struct RendererStorage;
		friend struct RendererDrawList;
		friend class Animator;
		friend class GraphicsPipeline;
	};
}