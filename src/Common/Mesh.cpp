#include "stdafx.h"
#include "Common/Mesh.h"
#include "Common/VertexArray.h"
#include "Common/VertexBuffer.h"
#include "Common/IndexBuffer.h"
#include "Common/Material.h"

#include "Utils/Utils.h"
#include "Utils/glTFImporter.h"

namespace Frostium
{
    void Mesh::Create(const std::string& filePath, Mesh* obj)
    {
        if (obj)
        {
            if (obj->m_Initialized)
                return;

            obj->m_ImportedData = new ImportedDataGlTF();
            if (glTFImporter::Import(filePath, obj->m_ImportedData))
            {
                Init(obj, &obj->m_ImportedData->Primitives[0]);

                obj->m_Meshes.resize(obj->m_ImportedData->Primitives.size() - 1);
                for (uint32_t i = 0; i < static_cast<uint32_t>(obj->m_ImportedData->Primitives.size() - 1); ++i)
                {
                    Primitive& primitve = obj->m_ImportedData->Primitives[i + 1];
                    Init(&obj->m_Meshes[i], &primitve);
                }
            }
            delete obj->m_ImportedData;
        }
    }

    void Mesh::SetMaterialID(int32_t materialID, uint32_t meshIndex)
    {
        if (m_Meshes.size() < meshIndex)
            m_Meshes[meshIndex].m_MaterialID = materialID;
    }

    void Mesh::SetMaterialID(int32_t materialID, const std::string& meshName)
    {

    }

    void Mesh::SetMaterialID(int32_t materialID)
    {
        m_MaterialID = materialID;
        for (Mesh& mesh : m_Meshes)
            mesh.m_MaterialID = materialID;
    }

    bool Mesh::Init(Mesh* mesh, Primitive* primitive)
    {
        mesh->m_VertexBuffer = std::make_unique<VertexBuffer>();
        mesh->m_IndexBuffer =  std::make_unique<IndexBuffer>();
        mesh->m_VertexCount =  static_cast<uint32_t>(primitive->VertexBuffer.size());

        VertexBuffer::Create(mesh->m_VertexBuffer.get(), primitive->VertexBuffer.data(), static_cast<uint32_t>(primitive->VertexBuffer.size() * sizeof(PBRVertex)), true);
        IndexBuffer::Create(mesh->m_IndexBuffer.get(), primitive->IndexBuffer.data(), static_cast<uint32_t>(primitive->IndexBuffer.size()), true);
        mesh->m_Initialized = true;
        return true;
    }
}