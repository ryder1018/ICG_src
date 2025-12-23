#include "header/animated_model.h"
#include "header/stb_image.h"
#include <iostream>
#include <filesystem>


AnimatedModel::AnimatedModel(const std::string& path) : texture(0) {
    loadModel(path);
}

void AnimatedModel::loadModel(const std::string& path) {
    m_scene = m_Importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    
    if (!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode) {
        std::cout << "ERROR::ASSIMP:: " << m_Importer.GetErrorString() << std::endl;
        return;
    }
    
    // Initialize bone matrices
    // m_FinalBoneMatrices.resize(100, glm::mat4(1.0f)); // Reserve space for 100 bones
    
    // Set current animation if available
    if (m_scene->mNumAnimations > 0) {
        m_CurrentAnimation = m_scene->mAnimations[0];
    }
    
    processNode(m_scene->mRootNode, m_scene);
    
    // Resize based on actual bone count found
    if (m_BoneCounter > 0) {
        m_FinalBoneMatrices.resize(m_BoneCounter, glm::mat4(1.0f));
    } else {
        m_FinalBoneMatrices.resize(200, glm::mat4(1.0f));
    }

    setupMesh();
    loadMaterialTextures(m_scene);
}

void AnimatedModel::processNode(aiNode* node, const aiScene* scene) {
    // Process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }
    
    // Recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void AnimatedModel::processMesh(aiMesh* mesh, const aiScene* scene) {
    // Get the current number of vertices in the global buffer before adding new ones
    unsigned int baseVertex = vertices.size();

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        setVertexBoneDataToDefault(vertex);
        
        // Position
        vertex.Position.x = mesh->mVertices[i].x;
        vertex.Position.y = mesh->mVertices[i].y;
        vertex.Position.z = mesh->mVertices[i].z;
        
        // Normals
        if (mesh->HasNormals()) {
            vertex.Normal.x = mesh->mNormals[i].x;
            vertex.Normal.y = mesh->mNormals[i].y;
            vertex.Normal.z = mesh->mNormals[i].z;
        }
        
        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoords.x = mesh->mTextureCoords[0][i].x;
            vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
        } else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }
        
        vertices.push_back(vertex);
    }
    
    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j] + baseVertex); // Apply offset!
    }
    
    // Process bone weights
    extractBoneWeightForVertices(vertices, mesh, scene);
}

void AnimatedModel::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    // Vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // Vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    
    // Vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    
    // Bone IDs
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
    
    // Bone weights
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
    
    glBindVertexArray(0);
}

void AnimatedModel::loadTexture(const std::string& filepath) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture: " << filepath << std::endl;
    }
    stbi_image_free(data);
}

std::string AnimatedModel::strReplace(std::string str, const std::string& oldStr, const std::string& newStr) {
    size_t pos = 0;
    while((pos = str.find(oldStr, pos)) != std::string::npos) {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
    return str;
}

void AnimatedModel::loadMaterialTextures(const aiScene* scene) {
    if (!scene->HasMaterials()) return;
    
    // We only support one texture for now (the first diffuse texture found)
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* material = scene->mMaterials[i];
        
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString str;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
            std::string path = str.C_Str();
            
            // Check if texture is embedded
            const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(path.c_str());
            
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            unsigned char* data = nullptr;
            int width, height, nrChannels;
            stbi_set_flip_vertically_on_load(false);
            
            if (embeddedTexture) {
                // Load embedded texture
                if (embeddedTexture->mHeight == 0) {
                    // Compressed texture (e.g. PNG, JPG inside FBX)
                    data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embeddedTexture->pcData), embeddedTexture->mWidth, &width, &height, &nrChannels, 0);
                } else {
                    // Raw texture data (ARGB8888)
                    data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(embeddedTexture->pcData), embeddedTexture->mWidth * embeddedTexture->mHeight * 4, &width, &height, &nrChannels, 0);
                }
                
                if (data) std::cout << "Loaded embedded texture from FBX!" << std::endl;
            } else {
                // Load from file path (handle potential path issues)
                // Fix path: Assimp might return full absolute paths from original PC, take filename only
                std::string filename = path;
                const size_t last_slash_idx = filename.find_last_of("\\/");
                if (std::string::npos != last_slash_idx) {
                    filename = filename.substr(last_slash_idx + 1);
                }
                
                // Try texture directory first
                std::string textureDir = "../../src/asset/texture/";
                std::string fullPath = textureDir + filename;
                
                data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);
                
                if (!data) {
                    // Fallback to searching in same dir as model or just filename
                    data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
                }
                
                if (data) std::cout << "Loaded referenced texture: " << filename << std::endl;
            }
            
            if (data) {
                GLenum format;
                if (nrChannels == 1) format = GL_RED;
                else if (nrChannels == 3) format = GL_RGB;
                else if (nrChannels == 4) format = GL_RGBA;
                
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data);
                
                // We found a texture, stop searching (simplification for single-texture model)
                return; 
            } else {
                std::cout << "Failed to load FBX texture: " << path << std::endl;
            }
        }
    }
}

void AnimatedModel::render() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void AnimatedModel::updateAnimation(float timeInSeconds) {
    if (!m_CurrentAnimation) return;
    
    m_AnimationTime = fmod(timeInSeconds * m_CurrentAnimation->mTicksPerSecond, m_CurrentAnimation->mDuration);
    calculateBoneTransform(m_scene->mRootNode, glm::mat4(1.0f), m_AnimationTime);
}

void AnimatedModel::calculateBoneTransform(const aiNode* node, glm::mat4 parentTransform, float animationTime) {
    std::string nodeName = node->mName.data;
    glm::mat4 nodeTransform = aiMatrix4x4ToGlm(node->mTransformation);
    
    const aiNodeAnim* nodeAnim = nullptr;
    
    // Find the animation node for this bone
    if (m_CurrentAnimation) {
        for (unsigned int i = 0; i < m_CurrentAnimation->mNumChannels; i++) {
            if (std::string(m_CurrentAnimation->mChannels[i]->mNodeName.data) == nodeName) {
                nodeAnim = m_CurrentAnimation->mChannels[i];
                break;
            }
        }
    }
    
    if (nodeAnim) {
        // Interpolate scaling and generate scaling transformation matrix
        glm::vec3 scaling(1.0f);
        if (nodeAnim->mNumScalingKeys == 1) {
            scaling = aiVector3DToGlm(nodeAnim->mScalingKeys[0].mValue);
        } else if (nodeAnim->mNumScalingKeys > 1) {
            unsigned int scalingIndex = 0;
            bool foundScaling = false;
            for (unsigned int i = 0; i < nodeAnim->mNumScalingKeys - 1; i++) {
                if (animationTime < (float)nodeAnim->mScalingKeys[i + 1].mTime) {
                    scalingIndex = i;
                    foundScaling = true;
                    break;
                }
            }
            
            if (!foundScaling) {
                scaling = aiVector3DToGlm(nodeAnim->mScalingKeys[nodeAnim->mNumScalingKeys - 1].mValue);
            } else {
                float deltaTime = nodeAnim->mScalingKeys[scalingIndex + 1].mTime - nodeAnim->mScalingKeys[scalingIndex].mTime;
                float factor = (deltaTime > 0.0f)
                    ? (animationTime - (float)nodeAnim->mScalingKeys[scalingIndex].mTime) / deltaTime
                    : 0.0f;
                factor = glm::clamp(factor, 0.0f, 1.0f);
                
                glm::vec3 start = aiVector3DToGlm(nodeAnim->mScalingKeys[scalingIndex].mValue);
                glm::vec3 end = aiVector3DToGlm(nodeAnim->mScalingKeys[scalingIndex + 1].mValue);
                scaling = glm::mix(start, end, factor);
            }
        }
        
        // Interpolate rotation and generate rotation transformation matrix
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (nodeAnim->mNumRotationKeys == 1) {
            rotation = aiQuaternionToGlm(nodeAnim->mRotationKeys[0].mValue);
        } else if (nodeAnim->mNumRotationKeys > 1) {
            unsigned int rotationIndex = 0;
            bool foundRotation = false;
            for (unsigned int i = 0; i < nodeAnim->mNumRotationKeys - 1; i++) {
                if (animationTime < (float)nodeAnim->mRotationKeys[i + 1].mTime) {
                    rotationIndex = i;
                    foundRotation = true;
                    break;
                }
            }
            
            if (!foundRotation) {
                rotation = aiQuaternionToGlm(nodeAnim->mRotationKeys[nodeAnim->mNumRotationKeys - 1].mValue);
            } else {
                float deltaTime = nodeAnim->mRotationKeys[rotationIndex + 1].mTime - nodeAnim->mRotationKeys[rotationIndex].mTime;
                float factor = (deltaTime > 0.0f)
                    ? (animationTime - (float)nodeAnim->mRotationKeys[rotationIndex].mTime) / deltaTime
                    : 0.0f;
                factor = glm::clamp(factor, 0.0f, 1.0f);
                
                glm::quat start = aiQuaternionToGlm(nodeAnim->mRotationKeys[rotationIndex].mValue);
                glm::quat end = aiQuaternionToGlm(nodeAnim->mRotationKeys[rotationIndex + 1].mValue);
                rotation = glm::slerp(start, end, factor);
            }
        }
        
        // Interpolate translation and generate translation transformation matrix
        glm::vec3 translation(0.0f);
        if (nodeAnim->mNumPositionKeys == 1) {
            translation = aiVector3DToGlm(nodeAnim->mPositionKeys[0].mValue);
        } else if (nodeAnim->mNumPositionKeys > 1) {
            unsigned int positionIndex = 0;
            bool foundPosition = false;
            for (unsigned int i = 0; i < nodeAnim->mNumPositionKeys - 1; i++) {
                if (animationTime < (float)nodeAnim->mPositionKeys[i + 1].mTime) {
                    positionIndex = i;
                    foundPosition = true;
                    break;
                }
            }
            
            if (!foundPosition) {
                translation = aiVector3DToGlm(nodeAnim->mPositionKeys[nodeAnim->mNumPositionKeys - 1].mValue);
            } else {
                float deltaTime = nodeAnim->mPositionKeys[positionIndex + 1].mTime - nodeAnim->mPositionKeys[positionIndex].mTime;
                float factor = (deltaTime > 0.0f)
                    ? (animationTime - (float)nodeAnim->mPositionKeys[positionIndex].mTime) / deltaTime
                    : 0.0f;
                factor = glm::clamp(factor, 0.0f, 1.0f);
                
                glm::vec3 start = aiVector3DToGlm(nodeAnim->mPositionKeys[positionIndex].mValue);
                glm::vec3 end = aiVector3DToGlm(nodeAnim->mPositionKeys[positionIndex + 1].mValue);
                translation = glm::mix(start, end, factor);
            }
        }
        
        // Combine transformations
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scaling);
        
        nodeTransform = translationMatrix * rotationMatrix * scaleMatrix;
    }
    
    glm::mat4 globalTransformation = parentTransform * nodeTransform;
    
    auto boneInfoMap = m_BoneInfoMap.find(nodeName);
    if (boneInfoMap != m_BoneInfoMap.end()) {
        int index = boneInfoMap->second.id;
        glm::mat4 offset = boneInfoMap->second.offset;
        m_FinalBoneMatrices[index] = globalTransformation * offset;
    }
    
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        calculateBoneTransform(node->mChildren[i], globalTransformation, animationTime);
    }
}

glm::mat4 AnimatedModel::aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

glm::vec3 AnimatedModel::aiVector3DToGlm(const aiVector3D& vec) {
    return glm::vec3(vec.x, vec.y, vec.z);
}

glm::quat AnimatedModel::aiQuaternionToGlm(const aiQuaternion& pOrientation) {
    return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
}

void AnimatedModel::setVertexBoneDataToDefault(Vertex& vertex) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        vertex.m_BoneIDs[i] = -1;
        vertex.m_Weights[i] = 0.0f;
    }
}

void AnimatedModel::setVertexBoneData(Vertex& vertex, int boneID, float weight) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        if (vertex.m_BoneIDs[i] < 0) {
            vertex.m_Weights[i] = weight;
            vertex.m_BoneIDs[i] = boneID;
            break;
        }
    }
}

void AnimatedModel::extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene) {
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
        
        if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end()) {
            BoneInfo newBoneInfo;
            newBoneInfo.id = m_BoneCounter;
            newBoneInfo.offset = aiMatrix4x4ToGlm(mesh->mBones[boneIndex]->mOffsetMatrix);
            m_BoneInfoMap[boneName] = newBoneInfo;
            boneID = m_BoneCounter;
            m_BoneCounter++;
        } else {
            boneID = m_BoneInfoMap[boneName].id;
        }
        
        assert(boneID != -1);
        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;
        
        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexId <= vertices.size());
            setVertexBoneData(vertices[vertexId], boneID, weight);
        }
    }
}
