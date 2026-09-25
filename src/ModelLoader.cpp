#include "ModelLoader.h"

#include <tiny_obj_loader.h>

#include <filesystem>
#include <stdexcept>
#include <unordered_map>

namespace fs = std::filesystem;

ModelData ModelLoader::load(const std::string& path)
{
    const fs::path modelPath(path);

    if (!fs::exists(modelPath)) {
        throw std::runtime_error("Model file does not exist: " + path);
    }

    const std::string extension = modelPath.extension().string();

    if (extension == ".obj" || extension == ".OBJ") {
        return loadOBJ(path);
    }

#ifdef HAVE_ASSIMP
    return loadAssimp(path);
#else
    throw std::runtime_error(
        "Unsupported model format: " + extension +
        " (FBX support requires Assimp)"
    );
#endif
}

ModelData ModelLoader::loadOBJ(const std::string& path)
{
    ModelData model;
    model.sourcePath = path;

    const fs::path modelPath(path);
    const fs::path modelDirectory =
        modelPath.has_parent_path()
            ? modelPath.parent_path()
            : fs::current_path();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warning;
    std::string error;

    const bool loaded = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &warning,
        &error,
        path.c_str(),
        modelDirectory.string().c_str(),
        true
    );

    if (!warning.empty()) {
        // Warnings are intentionally not fatal.
        // The viewer can still display a partially valid OBJ.
    }

    if (!loaded) {
        throw std::runtime_error(
            "Failed to load OBJ '" + path + "': " + error
        );
    }

    // ---------------------------------------------------------------------
    // Materials
    // ---------------------------------------------------------------------

    model.materials.reserve(materials.size());

    for (const tinyobj::material_t& sourceMaterial : materials) {
        MaterialData material;
        material.name = sourceMaterial.name;

        material.fallbackColor = glm::vec3(
            sourceMaterial.diffuse[0],
            sourceMaterial.diffuse[1],
            sourceMaterial.diffuse[2]
        );

        if (material.fallbackColor.x <= 0.0f &&
            material.fallbackColor.y <= 0.0f &&
            material.fallbackColor.z <= 0.0f) {
            material.fallbackColor = glm::vec3(0.85f);
        }

        if (!sourceMaterial.diffuse_texname.empty()) {
            fs::path texturePath(sourceMaterial.diffuse_texname);

            if (texturePath.is_relative()) {
                texturePath = modelDirectory / texturePath;
            }

            material.diffuseTexturePath =
                fs::weakly_canonical(texturePath).string();
        }

        model.materials.push_back(std::move(material));
    }

    // ---------------------------------------------------------------------
    // Geometry
    // ---------------------------------------------------------------------

    for (const tinyobj::shape_t& shape : shapes) {
        MeshData mesh;

        // OBJ files can contain multiple materials inside one shape.
        // Split the shape into one MeshData per material so each draw call
        // can use the correct texture.
        std::unordered_map<int, size_t> materialMeshes;

        size_t indexOffset = 0;

        for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); ++face) {
            const unsigned char vertexCount =
                shape.mesh.num_face_vertices[face];

            if (vertexCount < 3) {
                indexOffset += vertexCount;
                continue;
            }

            const int materialId =
                shape.mesh.material_ids.empty()
                    ? -1
                    : shape.mesh.material_ids[face];

            auto it = materialMeshes.find(materialId);

            size_t meshIndex;

            if (it == materialMeshes.end()) {
                MeshData newMesh;
                newMesh.materialIndex = materialId;

                model.meshes.push_back(std::move(newMesh));
                meshIndex = model.meshes.size() - 1;

                materialMeshes.emplace(materialId, meshIndex);
            } else {
                meshIndex = it->second;
            }

            MeshData& targetMesh = model.meshes[meshIndex];

            // TinyObjLoader normally triangulates when triangulate=true,
            // but handle polygons defensively using a triangle fan.
            for (size_t triangleVertex = 1;
                 triangleVertex + 1 < vertexCount;
                 ++triangleVertex) {

                const size_t localIndices[3] = {
                    0,
                    triangleVertex,
                    triangleVertex + 1
                };

                for (size_t corner = 0; corner < 3; ++corner) {
                    const tinyobj::index_t index =
                        shape.mesh.indices[
                            indexOffset + localIndices[corner]
                        ];

                    Vertex vertex{};

                    // Position
                    if (index.vertex_index >= 0 &&
                        static_cast<size_t>(3 * index.vertex_index + 2)
                            < attrib.vertices.size()) {

                        vertex.pos = glm::vec3(
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                        );
                    }

                    // Normal
                    if (index.normal_index >= 0 &&
                        static_cast<size_t>(3 * index.normal_index + 2)
                            < attrib.normals.size()) {

                        vertex.normal = glm::vec3(
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]
                        );
                    } else {
                        vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    }

                    // UV
                    if (index.texcoord_index >= 0 &&
                        static_cast<size_t>(2 * index.texcoord_index + 1)
                            < attrib.texcoords.size()) {

                        vertex.uv = glm::vec2(
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f -
                            attrib.texcoords[2 * index.texcoord_index + 1]
                        );
                    } else {
                        vertex.uv = glm::vec2(0.0f);
                    }

                    targetMesh.indices.push_back(
                        static_cast<uint32_t>(targetMesh.vertices.size())
                    );

                    targetMesh.vertices.push_back(vertex);
                }
            }

            indexOffset += vertexCount;
        }
    }

    // Remove empty meshes. uploadMesh() cannot create a zero-sized Vulkan
    // buffer, so keeping them around would only cause trouble later.
    model.meshes.erase(
        std::remove_if(
            model.meshes.begin(),
            model.meshes.end(),
            [](const MeshData& mesh) {
                return mesh.vertices.empty() || mesh.indices.empty();
            }
        ),
        model.meshes.end()
    );

    if (model.meshes.empty()) {
        throw std::runtime_error(
            "OBJ contains no renderable geometry: " + path
        );
    }

    return model;
}

#ifdef HAVE_ASSIMP

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace {

void processAssimpNode(
    const aiNode* node,
    const aiScene* scene,
    const fs::path& modelDirectory,
    ModelData& model)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh* sourceMesh = scene->mMeshes[node->mMeshes[i]];

        MeshData mesh;
        mesh.materialIndex = static_cast<int>(sourceMesh->mMaterialIndex);

        mesh.vertices.reserve(sourceMesh->mNumVertices);

        for (unsigned int v = 0; v < sourceMesh->mNumVertices; ++v) {
            Vertex vertex{};

            const aiVector3D& position = sourceMesh->mVertices[v];
            vertex.pos = glm::vec3(
                position.x,
                position.y,
                position.z
            );

            if (sourceMesh->HasNormals()) {
                const aiVector3D& normal = sourceMesh->mNormals[v];

                vertex.normal = glm::vec3(
                    normal.x,
                    normal.y,
                    normal.z
                );
            } else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            if (sourceMesh->HasTextureCoords(0)) {
                const aiVector3D& uv =
                    sourceMesh->mTextureCoords[0][v];

                vertex.uv = glm::vec2(
                    uv.x,
                    1.0f - uv.y
                );
            } else {
                vertex.uv = glm::vec2(0.0f);
            }

            mesh.vertices.push_back(vertex);
        }

        for (unsigned int f = 0; f < sourceMesh->mNumFaces; ++f) {
            const aiFace& face = sourceMesh->mFaces[f];

            if (face.mNumIndices < 3) {
                continue;
            }

            // Triangulate arbitrary polygons with a triangle fan.
            for (unsigned int j = 1; j + 1 < face.mNumIndices; ++j) {
                mesh.indices.push_back(face.mIndices[0]);
                mesh.indices.push_back(face.mIndices[j]);
                mesh.indices.push_back(face.mIndices[j + 1]);
            }
        }

        if (!mesh.vertices.empty() && !mesh.indices.empty()) {
            model.meshes.push_back(std::move(mesh));
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processAssimpNode(
            node->mChildren[i],
            scene,
            modelDirectory,
            model
        );
    }
}

} // namespace

ModelData ModelLoader::loadAssimp(const std::string& path)
{
    ModelData model;
    model.sourcePath = path;

    const fs::path modelPath(path);
    const fs::path modelDirectory =
        modelPath.has_parent_path()
            ? modelPath.parent_path()
            : fs::current_path();

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality
    );

    if (!scene ||
        !scene->mRootNode ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {

        throw std::runtime_error(
            "Failed to load model '" + path + "': " +
            importer.GetErrorString()
        );
    }

    // Materials
    model.materials.reserve(scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial* sourceMaterial = scene->mMaterials[i];

        MaterialData material;

        aiString name;
        if (sourceMaterial->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            material.name = name.C_Str();
        }

        aiColor3D diffuseColor(0.85f, 0.85f, 0.85f);

        if (sourceMaterial->Get(
                AI_MATKEY_COLOR_DIFFUSE,
                diffuseColor) == AI_SUCCESS) {

            material.fallbackColor = glm::vec3(
                diffuseColor.r,
                diffuseColor.g,
                diffuseColor.b
            );
        }

        aiString texturePath;

        if (sourceMaterial->GetTexture(
                aiTextureType_DIFFUSE,
                0,
                &texturePath) == AI_SUCCESS) {

            fs::path texture = texturePath.C_Str();

            // Embedded textures are not handled yet.
            if (!texture.empty() && texture.string()[0] != '*') {
                if (texture.is_relative()) {
                    texture = modelDirectory / texture;
                }

                material.diffuseTexturePath =
                    fs::weakly_canonical(texture).string();
            }
        }

        model.materials.push_back(std::move(material));
    }

    processAssimpNode(
        scene->mRootNode,
        scene,
        modelDirectory,
        model
    );

    if (model.meshes.empty()) {
        throw std::runtime_error(
            "Model contains no renderable geometry: " + path
        );
    }

    return model;
}

#endif
