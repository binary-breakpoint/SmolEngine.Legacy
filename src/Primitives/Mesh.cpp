#include "stdafx.h"
#include "Primitives/Mesh.h"
#include "Primitives/VertexArray.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"

#include "Tools/Utils.h"
#include "Pools/MeshPool.h"
#include "Import/glTFImporter.h"

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
            // Root
            {
                std::hash<std::string_view> hasher{};
                Primitive* primitve = &data->Primitives[0];
                m_AABB = primitve->AABB;
                m_Name = primitve->MeshName;
                m_ID = hasher(path);
                Build(m_Root, nullptr, primitve);

                m_Scene.emplace_back(m_Root);
            }

            // Children
            uint32_t childCount = static_cast<uint32_t>(data->Primitives.size() - 1);
            m_Childs.resize(childCount);
            for (uint32_t i = 0; i < childCount; ++i)
            {
                Ref<Mesh> mesh = Mesh::Create();
                Primitive* primitve = &data->Primitives[i + 1];
                mesh->m_AABB = primitve->AABB;
                mesh->m_Name = primitve->MeshName;

                Build(mesh, m_Root, primitve);

                m_Childs[i] = mesh;
                m_Scene.emplace_back(mesh);
            }
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
        return m_ID;
    }

    std::string Mesh::GetName() const
    {
        return m_Name;
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
}