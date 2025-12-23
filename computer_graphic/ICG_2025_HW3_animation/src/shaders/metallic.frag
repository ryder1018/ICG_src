#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} fs_in;

in float isAura;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    float gloss;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
};

uniform Light light;
uniform Material material;
uniform vec3 viewPos;
uniform samplerCube skybox;

// Hyperparameters
uniform float bias;            // 0.2
uniform float alpha;           // 0.4
uniform float lightIntensity;  // 1.0

void main() 
{
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 lightDir = normalize(light.position - fs_in.FragPos);

    float diff = max(dot(norm, lightDir), 0.0);

    // No albedo/texture contribution for metallic mode
    vec3 ambient = light.ambient * material.ambient * 0.3; // keep base light subtle
    float diffuseWeight = 0.1; // further suppress diffuse for a mirror-like metal
    vec3 diffuse = lightIntensity * light.diffuse * material.diffuse * diff * diffuseWeight;

    // Sample environment via reflection vector
    // Environment reflection
    vec3 reflectDir = reflect(-viewDir, norm);
    vec3 envColor = texture(skybox, reflectDir).rgb * 1.5; 

    // Fresnel
    float cosTheta = clamp(dot(viewDir, norm), 0.0, 1.0);
    float fresnelScale = 0.7;
    float fresnelPower = max(alpha, 0.0001); 
    float F0 = clamp(bias, 0.0, 1.0);
    float reflectWeight = clamp(F0 + fresnelScale * pow(1.0 - cosTheta, fresnelPower), 0.0, 1.0);

    vec3 finalColor = mix(ambient + diffuse, envColor, reflectWeight);
    
    // Aura Handling
    float finalAlpha = 1.0; // Default to Solid for Base
    if (isAura > 0.5) {
        // Aura Mode: Tint Cyan, Lower Alpha
        finalColor = mix(finalColor, vec3(0.0, 1.0, 1.0), 0.5); 
        finalAlpha = 0.3; 
    }

    FragColor = vec4(finalColor, finalAlpha);
} 
