#include "stdafx.h"
#include "Utils/ModelImporter.h"
#include "Common/SLog.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/gtc/type_ptr.hpp>

namespace Frostium
{
    // TODO: Add materials

    aiMatrix4x4 GLMMat4ToAi(const glm::mat4& mat)
    {
        return aiMatrix4x4(mat[0][0], mat[0][1], mat[0][2], mat[0][3],
            mat[1][0], mat[1][1], mat[1][2], mat[1][3],
            mat[2][0], mat[2][1], mat[2][2], mat[2][3],
            mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
    }

    glm::mat4 AiToGLMMat4(const aiMatrix4x4& in_mat)
    {
        glm::mat4 tmp;
        tmp[0][0] = in_mat.a1;
        tmp[1][0] = in_mat.b1;
        tmp[2][0] = in_mat.c1;
        tmp[3][0] = in_mat.d1;

        tmp[0][1] = in_mat.a2;
        tmp[1][1] = in_mat.b2;
        tmp[2][1] = in_mat.c2;
        tmp[3][1] = in_mat.d2;

        tmp[0][2] = in_mat.a3;
        tmp[1][2] = in_mat.b3;
        tmp[2][2] = in_mat.c3;
        tmp[3][2] = in_mat.d3;

        tmp[0][3] = in_mat.a4;
        tmp[1][3] = in_mat.b4;
        tmp[2][3] = in_mat.c4;
        tmp[3][3] = in_mat.d4;
        return tmp;
    }

    glm::vec3 vec3_cast(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
    glm::vec2 vec2_cast(const aiVector3D& v) { return glm::vec2(v.x, v.y); } // it's aiVector3D because assimp's texture coordinates use that
    glm::quat quat_cast(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }
    glm::mat4 mat4_cast(const aiMatrix4x4& m) { return glm::transpose(glm::make_mat4(&m.a1)); }
    glm::mat4 mat4_cast(const aiMatrix3x3& m) { return glm::transpose(glm::make_mat3(&m.a1)); }

    void ProcessMesh(aiMesh** meshes, const aiNode* node, const aiNode* parent, ImportedData* out_data)
    {
        if (node->mNumMeshes > 0)
        {
            ImportedComponent component = {};
            uint32_t vertexCount = 0;
            uint32_t indexCount = 0;

            for (uint32_t y = 0; y < node->mNumMeshes; ++y)
            {
                uint32_t id = node->mMeshes[y];
                const aiMesh* mesh = meshes[id];

                vertexCount += mesh->mNumVertices;
                indexCount += mesh->mNumFaces;
            }

            glm::mat4 Transformation = AiToGLMMat4(node->mTransformation);
            if (parent)
            {
                Transformation *= AiToGLMMat4(parent->mTransformation);
            }

            component.VertexData.reserve(vertexCount);
            component.Indices.reserve(indexCount * 3);

            for (uint32_t y = 0; y < node->mNumMeshes; ++y)
            {
                uint32_t id = node->mMeshes[y];
                const aiMesh* mesh = meshes[id];

                for (uint32_t x = 0; x < mesh->mNumVertices; ++x)
                {
                    PBRVertex data = {};

                    // Position
                    data.Pos = glm::vec4(mesh->mVertices[x].x, mesh->mVertices[x].y, mesh->mVertices[x].z, 1) * Transformation;

                    // Normals
                    if (mesh->HasNormals())
                        data.Normals = { mesh->mNormals[x].x, mesh->mNormals[x].y, mesh->mNormals[x].z };

                    // UVs
                    if (mesh->HasTextureCoords(0))
                        data.UVs = { mesh->mTextureCoords[0][x].x, mesh->mTextureCoords[0][x].y };

                    //Tangents
                    if (mesh->HasTangentsAndBitangents())
                        data.Tangent = { mesh->mTangents[x].x, mesh->mTangents[x].y, mesh->mTangents[x].z };

                    component.VertexData.emplace_back(data);
                }

                for (uint32_t x = 0; x < mesh->mNumFaces; ++x)
                {
                    component.Indices.emplace_back(mesh->mFaces[x].mIndices[0]);
                    component.Indices.emplace_back(mesh->mFaces[x].mIndices[1]);
                    component.Indices.emplace_back(mesh->mFaces[x].mIndices[2]);
                }
            }

            component.Name = node->mName.C_Str();
            out_data->Components.emplace_back(component);
        }

       
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
            ProcessMesh(meshes, node->mChildren[i], node, out_data);
    }

    bool ModelImporter::Load(const std::string& filePath, ImportedData* out_data, ModelImporterFlags flags)
    {
        Assimp::Importer* importer = new Assimp::Importer();
        const aiScene* g_scene = importer->ReadFile(filePath.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
        auto meshes = g_scene->mMeshes;

        uint32_t childsCount = 0;
        for (uint32_t i = 0; i < g_scene->mRootNode->mNumChildren; ++i)
        {
            const aiNode* node = g_scene->mRootNode->mChildren[i];
            if (node->mNumMeshes > 0)
                childsCount++;

            for (uint32_t j = 0; j < node->mNumChildren; j++)
            {
                auto child = node->mChildren[j];
                if (child->mNumMeshes > 0)
                    childsCount++;
            }
        }
        out_data->Components.reserve(childsCount);

        for (uint32_t i = 0; i < g_scene->mRootNode->mNumChildren; ++i)
        {
            const aiNode* node = g_scene->mRootNode->mChildren[i];
            ProcessMesh(meshes, node, nullptr, out_data);
        }

        delete importer;
        return true;
	}
}