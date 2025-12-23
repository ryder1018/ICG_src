#ifndef ANIMATED_MODEL_H
#define ANIMATED_MODEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <map>

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct BoneInfo {
    int id;
    glm::mat4 offset;
};

class AnimatedModel {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
    unsigned int texture;
    
    // bone stuff
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
    
    // animation
    const aiScene* m_scene;
    Assimp::Importer m_Importer;
    
    AnimatedModel(const std::string& path);
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);
    void loadTexture(const std::string& filepath);
    void setupMesh();
    void render();
    
    // animation functions
    void updateAnimation(float timeInSeconds);
    void calculateBoneTransform(const aiNode* node, glm::mat4 parentTransform, float animationTime);
    glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from);
    glm::vec3 aiVector3DToGlm(const aiVector3D& vec);
    glm::quat aiQuaternionToGlm(const aiQuaternion& pOrientation);
    
    // bone functions
    void setVertexBoneDataToDefault(Vertex& vertex);
    void setVertexBoneData(Vertex& vertex, int boneID, float weight);
    void loadMaterialTextures(const aiScene* scene);
    std::string strReplace(std::string str, const std::string& oldStr, const std::string& newStr);
    void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
    
    std::vector<glm::mat4> m_FinalBoneMatrices;
    
private:
    float m_AnimationTime = 0.0f;
    aiAnimation* m_CurrentAnimation = nullptr;
};

#endif
