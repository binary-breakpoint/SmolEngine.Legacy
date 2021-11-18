#include "stdafx.h"
#include "Primitives/Mesh.h"
#include "Primitives/VertexArray.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"

#include "Tools/Utils.h"
#include "Import/glTFImporter.h"
#include "Materials/PBRFactory.h"
#include "Materials/MaterialPBR.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/json.hpp>

namespace SmolEngine
{
    void Mesh::Free()
    {
        for (auto& mesh : m_Scene)
        {
            mesh->m_VertexBuffer->Free();
            mesh->m_IndexBuffer->Free();
        }

        m_Scene.clear();
        m_Childs.clear();
    }

    bool Mesh::IsGood() const
    {
        return m_VertexBuffer->GetVertexCount() > 0;
    }

    bool Mesh::LoadFromFile(const std::string& path)
    {
        ImportedDataGlTF* data = new ImportedDataGlTF();
        const bool is_succeed = glTFImporter::Import(path, data);
        if (is_succeed)
        {
            uint32_t meshCount = static_cast<uint32_t>(data->Primitives.size());

            // Root
            {
                std::hash<std::string_view> hasher{};
                Primitive* primitve = &data->Primitives[0];

                m_Index = 0;
                m_AABB = primitve->AABB;
                m_Name = primitve->MeshName;
                m_ID = hasher(path);

                Build(m_Root, nullptr, primitve);

                m_SceneAABB.MaxPoint(m_AABB.MaxPoint());
                m_SceneAABB.MinPoint(m_AABB.MinPoint());

                m_Scene.emplace_back(m_Root);
            }

            // Children
            uint32_t childCount = static_cast<uint32_t>(meshCount - 1);
            m_Childs.resize(childCount);
            for (uint32_t i = 0; i < childCount; ++i)
            {
                Ref<Mesh> mesh = Mesh::Create();
                Primitive* primitve = &data->Primitives[i + 1];
                mesh->m_AABB = primitve->AABB;
                mesh->m_Name = primitve->MeshName;
                mesh->m_Index = i + 1;

                Build(mesh, m_Root, primitve);

                m_Childs[i] = mesh;

                m_SceneAABB.MaxPoint(mesh->m_AABB.MaxPoint());
                m_SceneAABB.MinPoint(mesh->m_AABB.MinPoint());

                m_Scene.emplace_back(mesh);
            }

            m_DefaultView = std::make_shared<MeshView>();
            m_DefaultView->m_Elements.resize(meshCount);
        }

        delete data;
        return is_succeed;
    }

    std::vector<Ref<Mesh>>& Mesh::GetScene()
    {
        return m_Scene;
    }

    std::vector<Ref<Mesh>>& Mesh::GetChilds()
    {
        return m_Childs;
    }

    BoundingBox& Mesh::GetAABB()
    {
        return m_AABB;
    }

    uint32_t Mesh::GetChildCount() const
    {
        return static_cast<uint32_t>(m_Childs.size());
    }

    size_t Mesh::GetID() const
    {
        return m_Root->m_ID;
    }

    uint32_t Mesh::GetNodeIndex() const
    {
        return m_Index;
    }

    std::string Mesh::GetName() const
    {
        return m_Name;
    }

    Ref<MeshView> Mesh::CreateMeshView() const
    {
        Ref<MeshView> view = std::make_shared<MeshView>();
        *view = *m_DefaultView;
        return view;
    }

    Ref<VertexBuffer> Mesh::GetVertexBuffer()
    {
        return m_VertexBuffer;
    }

    Ref<IndexBuffer> Mesh::GetIndexBuffer()
    {
        return m_IndexBuffer;
    }

    Ref<Mesh> Mesh::GetMeshByName(const std::string& name)
    {
        if (m_Root != nullptr)
            m_Root->GetMeshByName(name);

        for (auto& child : m_Childs)
        {
            if (child->GetName() == name)
                return child;
        }

        return nullptr;
    }

    Ref<Mesh> Mesh::GetMeshByIndex(uint32_t index)
    {
        if (index < m_Childs.size())
            return m_Childs[index];

        return nullptr;
    }

    bool Mesh::IsRootNode() const
    {
        return m_Root == nullptr;
    }

    Ref<Mesh> Mesh::Create()
    {
        Ref<Mesh> mesh = std::make_shared<Mesh>();
        mesh->m_Root = mesh;
        return mesh;
    }

    bool Mesh::Build(Ref<Mesh>& mesh, Ref<Mesh> parent, Primitive* primitive)
    {
        const bool is_static = true;

        if(parent != nullptr)
            mesh->m_Root = parent;

        mesh->m_VertexBuffer = VertexBuffer::Create();
        mesh->m_VertexBuffer->BuildFromMemory(primitive->VertexBuffer.data(), primitive->VertexBuffer.size() * sizeof(PBRVertex), is_static);

        mesh->m_IndexBuffer = IndexBuffer::Create();
        mesh->m_IndexBuffer->BuildFromMemory(primitive->IndexBuffer.data(), primitive->IndexBuffer.size(), is_static);

        return true;
    }

    bool MeshView::Serialize(const std::string& path)
    {
        std::stringstream storage;
        {
            cereal::JSONOutputArchive output{ storage };
            serialize(output);
        }

        std::ofstream myfile(path);
        if (myfile.is_open())
        {
            myfile << storage.str();
            myfile.close();
            return true;
        }

        return false;
    }

    bool MeshView::Deserialize(const std::string& path)
    {
        std::stringstream storage;
        std::ifstream file(path);
        if (!file)
        {
            DebugLog::LogError("Could not open the file: {}", path);
            return false;
        }

        storage << file.rdbuf();
        {
            cereal::JSONInputArchive input{ storage };
            input(m_Elements);
        }

        return true;
    }

    bool MeshView::TryLoadMaterials()
    {
        bool any_loaded = false;
        for (auto& element : m_Elements)
        {
            if (!element.m_PBRMatPath.empty())
            {
                PBRCreateInfo matCI{};
                if (matCI.Load(element.m_PBRMatPath))
                {
                    element.m_PBRHandle = PBRFactory::AddMaterial(&matCI, element.m_PBRMatPath);
                    any_loaded = true;
                }
            }
        }

        return any_loaded;
    }

    void MeshView::SetAnimationController(const Ref<AnimationController>& contoller)
    {
        m_AnimationController = contoller;
    }

    void MeshView::SetPBRHandle(const Ref<PBRHandle>& handle, uint32_t nodeIndex)
    {
        auto& element = m_Elements[nodeIndex];

        element.m_PBRMatPath = handle->m_Path;
        element.m_PBRHandle = handle;
    }

    void MeshView::SetMaterial(const Ref<Material3D>& material, uint32_t nodeIndex)
    {
        m_Elements[nodeIndex].m_Material = material;
    }

    void MeshView::SetTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    {
        Utils::ComposeTransform(position, rotation, scale, m_ModelMatrix);
    }

    const glm::mat4& MeshView::GetTransform() const
    {
        return m_ModelMatrix;
    }

    Ref<AnimationController> MeshView::GetAnimationController() const
    {
        return m_AnimationController;
    }

    Ref<PBRHandle> MeshView::GetPBRHandle(uint32_t nodeIndex) const
    {
        return m_Elements[nodeIndex].m_PBRHandle;
    }

    Ref<Material3D> MeshView::GetMaterial(uint32_t nodeIndex) const
    {
        return m_Elements[nodeIndex].m_Material;
    }
}