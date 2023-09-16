/* Conquer Space
 * Copyright (C) 2021-2023 Conquer Space
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <map>
#include <string>
#include <vector>

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <glm/glm.hpp>

#include "engine/asset/assetmanager.h"
#include "engine/graphics/texture.h"

namespace cqsp::asset {
class IOSystem;
class IOStream : public Assimp::IOStream {
    friend class IOSystem;

 protected:
    explicit IOStream(IVirtualFilePtr _ivfp) : ivfp(_ivfp) {}

    IVirtualFilePtr ivfp;

 public:
    size_t Read(void* pvBuffer, size_t pSize, size_t pCount);
    size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) {
        return pSize;  // We won't ever write because we are only going to open game files
    }
    aiReturn Seek(size_t pOffset, aiOrigin pOrigin);
    size_t Tell() const;
    size_t FileSize() const;
    void Flush();

    void Close();
};

class IOSystem : public Assimp::IOSystem {
    VirtualMounter* mount;

 public:
    explicit IOSystem(VirtualMounter* mount);
    ~IOSystem() = default;

    bool Exists(const char* file) const override;
    char getOsSeparator() const override { return '/'; }

    Assimp::IOStream* Open(const char* pFile, const char* pMode) override;
    void Close(Assimp::IOStream* pFile) override;
};

class Material {
    std::vector<Texture> textures;
};

struct Vertex {
 public:
    static const int MAX_BONE_INFLUENCE = 4;
    // position
    glm::vec3 position;
    // normal
    glm::vec3 normal;
    // texCoords
    glm::vec2 texCoords;
    // tangent
    glm::vec3 tangent;
    // bitangent
    glm::vec3 bitangent;
    //bone indexes which will influence this vertex
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    //weights from each bone
    float m_Weights[MAX_BONE_INFLUENCE];
};

class ModelLoader {
 public:
    void LoadNode(aiNode* node, const aiScene* scene);
    void LoadMesh(aiMesh* mesh, const aiScene* scene);
    void LoadMaterials(const aiScene* scene);
    void LoadMaterial(aiMaterial* material);
    void LoadMaterialTexture(aiMaterial* material, const aiTextureType& type);
    void GenerateMesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    std::map<std::string, Texture> material_textures;
};
}  // namespace cqsp::asset
