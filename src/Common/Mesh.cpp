#include "stdafx.h"
#include "Common/Mesh.h"
#include "Common/VertexArray.h"
#include "Common/VertexBuffer.h"
#include "Common/IndexBuffer.h"

#include "Utils/Utils.h"
#include "Utils/glTFImporter.h"

namespace Frostium
{
    Mesh::~Mesh()
    {
        if (m_ImportedData != nullptr)
            delete m_ImportedData;
    }

    void Mesh::Create(const std::string& filePath, Mesh* obj)
    {
        if (obj)
        {
            if (obj->m_Initialized)
                return;

            obj->m_ImportedData = new ImportedDataGlTF();
            ImportedDataGlTF* data = obj->m_ImportedData;

            if (glTFImporter::Import(filePath, data))
            {
                // Root
                {
                    Primitive* primitve = &data->Primitives[0];
                    obj->m_MeshMap[primitve->MeshName] = obj;
                    Init(obj, primitve);
                }

                // Children
                uint32_t childCount = static_cast<uint32_t>(data->Primitives.size() - 1);
                obj->m_Meshes.resize(childCount);
                for (uint32_t i = 0; i < childCount; ++i)
                {
                    Mesh* mesh = &obj->m_Meshes[i];
                    Primitive* primitve = &data->Primitives[i + 1];

                    obj->m_MeshMap[primitve->MeshName] = mesh;
                    Init(mesh, primitve);
                }
            }

            data->Primitives.clear();
        }
    }

    void Mesh::SetMaterialID(int32_t materialID, bool apply_to_children)
    {
        m_MaterialID = materialID;
        if (apply_to_children)
        {
            for (Mesh& mesh : m_Meshes)
                mesh.m_MaterialID = materialID;
        }
    }

    void Mesh::PlayActiveAnimation()
    {
        m_PlayAnimations = true;
    }

    void Mesh::StopActiveAnimation()
    {
        m_PlayAnimations = false;
    }

    void Mesh::SetActiveAnimByIndex(uint32_t index)
    {
        if (index < m_ImportedData->Animations.size())
            m_ImportedData->ActiveAnimation = index;
    }

    void Mesh::UpdateAnimations(float deltaTime)
    {
        if (m_ImportedData->Animations.size() > 0)
            m_ImportedData->UpdateAnimation(deltaTime);
    }

    const std::vector<Mesh>& Mesh::GetMeshes() const
    {
        return m_Meshes;
    }

    const uint32_t Mesh::GetVertexCount() const
    {
        return m_VertexCount;
    }

    VertexBuffer* Mesh::GetVertexBuffer()
    {
        return m_VertexBuffer.get();
    }

    IndexBuffer* Mesh::GetIndexBuffer()
    {
        return m_IndexBuffer.get();
    }

    Animation* Mesh::GetCurrentAnim()
    {
        return &m_ImportedData->Animations[m_ImportedData->ActiveAnimation].Animation;
    }

    Animation* Mesh::GetAnimationByIndex(uint32_t index)
    {
        if (index < m_ImportedData->Animations.size())
            return &m_ImportedData->Animations[index].Animation;

        return nullptr;
    }

    Mesh* Mesh::GetMeshByName(const std::string& name)
    {
        auto& it = m_MeshMap.find(name);
        if (it != m_MeshMap.end())
            return it->second;

        return nullptr;
    }

    Mesh* Mesh::GetMeshByIndex(uint32_t index)
    {
        if (index < m_Meshes.size())
            return &m_Meshes[index];

        return nullptr;
    }

    bool Mesh::Init(Mesh* mesh, Primitive* primitive)
    {
        mesh->m_VertexBuffer = std::make_shared<VertexBuffer>();
        mesh->m_IndexBuffer =  std::make_shared<IndexBuffer>();
        mesh->m_VertexCount =  static_cast<uint32_t>(primitive->VertexBuffer.size());

        VertexBuffer::Create(mesh->m_VertexBuffer.get(), primitive->VertexBuffer.data(), static_cast<uint32_t>(primitive->VertexBuffer.size() * sizeof(PBRVertex)), true);
        IndexBuffer::Create(mesh->m_IndexBuffer.get(), primitive->IndexBuffer.data(), static_cast<uint32_t>(primitive->IndexBuffer.size()), true);
        mesh->m_Initialized = true;
        return true;
    }
}