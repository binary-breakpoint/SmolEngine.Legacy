#pragma once
#include "Common/Core.h"
#include "Common/Common.h"

#include "Common/VertexBuffer.h"
#include "Common/IndexBuffer.h"

#include <memory>

namespace Frostium
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
		// Factory
		static void Create(const std::string& filePath, Mesh* mesh);

		// Materials
		void SetMaterialID(int32_t materialID, uint32_t meshIndex);
		void SetMaterialID(int32_t materialID, const std::string& meshName);
		void SetMaterialID(int32_t materialID);

		// Animations

		// Getters
		VertexBuffer* GetVertexBuffer() { return m_VertexBuffer.get(); }
		IndexBuffer* GetIndexBuffer() { return m_IndexBuffer.get(); }
		const uint32_t GetVertexCount() const { return m_VertexCount; }

	private:

		static bool Init(Mesh* mesh, Primitive* primitive);

	private:

		bool                      m_Initialized = false;
		uint32_t                  m_MaterialID = 0;
		uint32_t                  m_VertexCount = 0;
		Scope<VertexBuffer>       m_VertexBuffer = nullptr;
		Scope<IndexBuffer>        m_IndexBuffer = nullptr;
		ImportedDataGlTF*         m_ImportedData = nullptr;
		std::vector<Mesh>         m_Meshes;

	private:

		friend class Renderer;
		friend class GraphicsPipeline;
	};
}