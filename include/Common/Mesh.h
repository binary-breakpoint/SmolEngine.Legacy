#pragma once
#include "Common/Core.h"
#include "Common/Common.h"

#include "Common/VertexBuffer.h"
#include "Common/IndexBuffer.h"
#include "Common/Animation.h"

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
		Mesh() = default;
		~Mesh();

		// Factory
		static void Create(const std::string& filePath, Mesh* mesh);

		// Materials
		void SetMaterialID(int32_t materialID, bool apply_to_children = false);

		// Animations
		void PlayActiveAnimation();
		void StopActiveAnimation();
		void SetActiveAnimByIndex(uint32_t index);

		// Getters
		const std::vector<Mesh>& GetMeshes() const;
		const uint32_t GetVertexCount() const;
		VertexBuffer* GetVertexBuffer();
		IndexBuffer* GetIndexBuffer();
		Animation* GetCurrentAnim();
		Animation* GetAnimationByIndex(uint32_t index);
		Mesh* GetMeshByName(const std::string& name);
		Mesh* GetMeshByIndex(uint32_t index);

	private:

		static bool Init(Mesh* mesh, Primitive* primitive);
		void UpdateAnimations(float deltaTime);

	private:

		bool                                        m_Initialized = false;
		bool                                        m_PlayAnimations = true;
		uint32_t                                    m_MaterialID = 0;
		uint32_t                                    m_VertexCount = 0;
		Ref<VertexBuffer>                           m_VertexBuffer = nullptr;
		Ref<IndexBuffer>                            m_IndexBuffer = nullptr;
		ImportedDataGlTF*                           m_ImportedData = nullptr;
		std::unordered_map<std::string, Mesh*>      m_MeshMap;
		std::vector<Mesh>                           m_Meshes;

	private:

		friend class Renderer;
		friend class Animator;
		friend class GraphicsPipeline;
	};
}