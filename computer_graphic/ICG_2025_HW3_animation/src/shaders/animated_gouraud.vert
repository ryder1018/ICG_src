#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

const int MAX_BONES = 200;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 finalBonesMatrices[MAX_BONES];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    float gloss;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light light;
uniform Material material;
uniform vec3 viewPos;

out VS_OUT {
    vec3 Color;
    vec2 TexCoord;
} vs_out;

void main()
{
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    
    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
    {
        if(aBoneIDs[i] == -1) 
            continue;
        if(aBoneIDs[i] >= MAX_BONES) 
        {
            totalPosition = vec4(aPos, 1.0f);
            totalNormal = aNormal;
            break;
        }
        vec4 localPosition = finalBonesMatrices[aBoneIDs[i]] * vec4(aPos, 1.0f);
        totalPosition += localPosition * aWeights[i];
        vec3 localNormal = mat3(finalBonesMatrices[aBoneIDs[i]]) * aNormal;
        totalNormal += localNormal * aWeights[i];
    }
    
    // If no bones affect this vertex, use original position and normal
    if(length(totalPosition) == 0.0)
    {
        totalPosition = vec4(aPos, 1.0f);
        totalNormal = aNormal;
    }

    vec4 worldPos = model * totalPosition;
    vec3 norm = normalize(mat3(model) * totalNormal);
    vec3 lightDir = normalize(light.position - worldPos.xyz);
    vec3 viewDir = normalize(viewPos - worldPos.xyz);
    vec3 halfDir = normalize(lightDir + viewDir);

    float diff = max(dot(norm, lightDir), 0.0);
    float spec = diff > 0.0 ? pow(max(dot(norm, halfDir), 0.0), material.gloss) : 0.0;

    vec3 ambient = light.ambient * material.ambient;
    vec3 diffuse = light.diffuse * material.diffuse * diff;
    vec3 specular = light.specular * material.specular * spec;

    gl_Position = projection * view * worldPos;
    vs_out.Color = ambient + diffuse + specular;
    vs_out.TexCoord = aTexCoord;
}
