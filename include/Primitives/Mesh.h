#pragma once
#include "Common/Common.h"

#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"

#include <memory>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct ImportedDataGlTF;
	struct Primitive;
	class VertexBuffer;
	class IndexBuffer;
	class BufferLayout;
	class Texture;
	class Material;

	class Mesh
	{
	public:
		Mesh() = default;
		~Mesh();

		// Getters		
		std::vector<Mesh*>&   GetScene();
		std::vector<Mesh>&    GetChilds();
		BoundingBox&          GetAABB();
		uint32_t              GetVertexCount() const;
		uint32_t              GetChildCount() const;
		uint32_t              GetMeshID() const;
		std::string           GetName() const;
		VertexBuffer*         GetVertexBuffer();
		IndexBuffer*          GetIndexBuffer();
		Mesh*                 GetMeshByName(const std::string& name);
		Mesh*                 GetMeshByIndex(uint32_t index);
		// Helpers			  
		bool                  IsRootNode() const;
		bool                  IsReady() const;
		// Factory			  
		static void           Create(const std::string& filePath, Mesh* mesh);

	private:
		static bool           Init(Mesh* mesh, Mesh* parent, Primitive* primitive);

	private:
		Mesh*                 m_Root = nullptr;
		Ref<VertexBuffer>     m_VertexBuffer = nullptr;
		Ref<IndexBuffer>      m_IndexBuffer = nullptr;
		uint32_t              m_VertexCount = 0;
		uint32_t              m_ID = 0;
		std::string           m_Name = "";
		BoundingBox           m_AABB = {};
		std::vector<Mesh>     m_Childs;
		std::vector<Mesh*>    m_Scene;

	private:
		friend struct RendererStorage;
		friend struct RendererDrawList;
		friend class Animator;
		friend class GraphicsPipeline;
	};
}