#include "stdafx.h"
#include "Common/Mesh.h"
#include "Common/VertexArray.h"
#include "Common/VertexBuffer.h"
#include "Common/IndexBuffer.h"
#include "Common/Material.h"

#include "Utils/Utils.h"
#include "Utils/ModelImporter.h"
#include "Utils/glTFImporter.h"

#include <glm/glm.hpp>

namespace Frostium
{
    Mesh::Mesh() {}

    Mesh::~Mesh()
    {
        Free();
        for (auto& s : m_SubMeshes)
            s->Free();
    }

    void Mesh::Create(const std::string& filePath, Mesh* out_mesh)
    {
        if (out_mesh)
        {
            if (out_mesh->m_Initialized)
                return;

            ImportedDataGlTF* data = new ImportedDataGlTF();
            if (glTFImporter::Import(filePath, data))
            {
                out_mesh->Init(data);
                out_mesh->m_Initialized = true;
            }
            delete data;
        }
    }

    Mesh* Mesh::FindSubMeshByIndex(uint32_t index)
    {
        if (index >= m_SubMeshes.size())
            return nullptr;

        return m_SubMeshes[index];
    }

    Mesh* Mesh::FindSubMeshByName(const std::string& name)
    {
        for (auto& sub : m_SubMeshes)
        {
            if (sub->m_Name == name)
                return sub;
        }

        return nullptr;
    }

    const std::string& Mesh::GetName() const
    {
        return m_Name;
    }

    void Mesh::Free()
    {
        if (m_IndexBuffer)
            delete m_IndexBuffer;

        if (m_VertexBuffer)
            delete m_VertexBuffer;
    }

    void Mesh::FindAllMeshes()
    {
        m_Meshes.resize(m_SubMeshes.size() + 1);
        uint32_t index = 0;
        m_Meshes[index] = this;
        index++;

        for (auto& sub : m_SubMeshes)
        {
            m_Meshes[index] = sub;
            index++;
        }
    }

    bool Mesh::Init(ImportedDataGlTF* data)
    {
        Free();

        m_VertexBuffer = new VertexBuffer();
        m_IndexBuffer = new IndexBuffer();
        m_VertexCount = static_cast<uint32_t>(data->VertexBuffer.size());

        VertexBuffer::Create(m_VertexBuffer, data->VertexBuffer.data(), static_cast<uint32_t>(data->VertexBuffer.size() * sizeof(PBRVertex)), true);
        IndexBuffer::Create(m_IndexBuffer, data->IndexBuffer.data(), static_cast<uint32_t>(data->IndexBuffer.size()), true);

        FindAllMeshes();
        return true;
    }
}