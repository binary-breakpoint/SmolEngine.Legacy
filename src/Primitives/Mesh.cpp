#include "stdafx.h"
#include "Primitives/Mesh.h"
#include "Primitives/VertexArray.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"

#include "Utils/Utils.h"
#include "Import/glTFImporter.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
    Mesh::~Mesh()
    {

    }

    void Mesh::Create(const std::string& filePath, Mesh* obj)
    {
        if (obj)
        {
            if (obj->m_Initialized)
                return;

            obj->m_ImportedData = std::make_shared<ImportedDataGlTF>();
            ImportedDataGlTF* data = obj->m_ImportedData.get();

            if (glTFImporter::Import(filePath, data))
            {
                // Root
                {
                    std::hash<std::string_view> hasher{};
                    Primitive* primitve = &data->Primitives[0];
                    obj->m_AABB = primitve->AABB;
                    obj->m_Name = primitve->MeshName;
                    obj->m_ID = static_cast<uint32_t>(hasher(filePath));
                    Init(obj, nullptr,  primitve);

                    obj->m_Scene.emplace_back(obj);
                }

                // Children
                uint32_t childCount = static_cast<uint32_t>(data->Primitives.size() - 1);
                obj->m_Childs.resize(childCount);
                for (uint32_t i = 0; i < childCount; ++i)
                {
                    Mesh* mesh = &obj->m_Childs[i];
                    Primitive* primitve = &data->Primitives[i + 1];
                    mesh->m_AABB = primitve->AABB;
                    mesh->m_Name = primitve->MeshName;

                    Init(mesh, obj, primitve);
                    obj->m_Scene.emplace_back(mesh);
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
            for (Mesh& mesh : m_Childs)
                mesh.m_MaterialID = materialID;
        }
    }

    void Mesh::SetActiveAnimation(uint32_t index)
    {
        if (index < m_ImportedData->Animations.size())
            m_ImportedData->ActiveAnimation = index;
    }

    void Mesh::UpdateAnimations()
    {
        if (m_ImportedData->Animations.size() > 0)
            m_ImportedData->UpdateAnimation();
    }

    std::vector<Mesh*>& Mesh::GetScene()
    {
        return m_Scene;
    }

    std::vector<Mesh>& Mesh::GetChilds()
    {
        return m_Childs;
    }

    BoundingBox& Mesh::GetAABB()
    {
        return m_AABB;
    }

    uint32_t Mesh::GetVertexCount() const
    {
        return m_VertexCount;
    }

    uint32_t Mesh::GetAnimationsCount() const
    {
        if (m_Root != nullptr)
            return m_Root->GetAnimationsCount();

        return static_cast<uint32_t>(m_ImportedData->Animations.size());
    }

    uint32_t Mesh::GetChildCount() const
    {
        return static_cast<uint32_t>(m_Childs.size());
    }

    uint32_t Mesh::GetMaterialID() const
    {
        return m_MaterialID;
    }

    uint32_t Mesh::GetMeshID() const
    {
        if (m_Root != nullptr)
            return m_Root->GetMeshID();

        return m_ID;
    }

    std::string Mesh::GetName() const
    {
        return m_Name;
    }

    VertexBuffer* Mesh::GetVertexBuffer()
    {
        return m_VertexBuffer.get();
    }

    IndexBuffer* Mesh::GetIndexBuffer()
    {
        return m_IndexBuffer.get();
    }

    Mesh* Mesh::GetMeshByName(const std::string& name)
    {
        if (m_Name == name)
            return this;

        if (m_Root != nullptr)
            m_Root->GetMeshByName(name);

        for (auto& child : m_Childs)
        {
            if (child.GetName() == name)
                return &child;
        }

        return nullptr;
    }

    Mesh* Mesh::GetMeshByIndex(uint32_t index)
    {
        if (index < m_Childs.size())
            return &m_Childs[index];

        return nullptr;
    }

    AnimationProperties* Mesh::GetAnimationProperties(uint32_t index) const
    {
        if (m_Root != nullptr)
            return m_Root->GetAnimationProperties(index);

        if (index < m_ImportedData->Animations.size())
            return &m_ImportedData->Animations[index].Properties;

        return nullptr;
    }

    bool Mesh::IsAnimated() const
    {
        if (m_Root != nullptr)
            return m_Root->IsAnimated();

        return m_ImportedData->Animations.size() > 0;
    }

    bool Mesh::IsRootNode() const
    {
        return m_Root == nullptr;
    }

    bool Mesh::IsReady() const
    {
        return m_VertexCount > 0;
    }

    void Mesh::ResetAnimation(uint32_t index)
    {
        if (m_Root != nullptr)
            m_Root->ResetAnimation(index);

        m_ImportedData->ResetAnimation(index);
    }

    bool Mesh::Init(Mesh* mesh, Mesh* parent, Primitive* primitive)
    {
        mesh->m_VertexBuffer = std::make_shared<VertexBuffer>();
        mesh->m_IndexBuffer =  std::make_shared<IndexBuffer>();
        mesh->m_VertexCount =  static_cast<uint32_t>(primitive->VertexBuffer.size());
        mesh->m_Root = parent;

        VertexBuffer::Create(mesh->m_VertexBuffer.get(), primitive->VertexBuffer.data(), static_cast<uint32_t>(primitive->VertexBuffer.size() * sizeof(PBRVertex)), true);
        IndexBuffer::Create(mesh->m_IndexBuffer.get(), primitive->IndexBuffer.data(), static_cast<uint32_t>(primitive->IndexBuffer.size()), true);
        mesh->m_Initialized = true;
        return true;
    }
}