#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} fs_in;

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

uniform Light    light;
uniform Material material;
uniform vec3     viewPos;
uniform sampler2D ourTexture;

void main()
{
    vec3 norm = normalize(fs_in.Normal);
    vec3 lightDir = normalize(light.position - fs_in.FragPos);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    // Diffuse quantization
    float diff = max(dot(norm, lightDir), 0.0);
    float diffuseLevels = 3.0;
    float diffStep = floor(diff * diffuseLevels) / diffuseLevels;

    // Specular toon band
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = diff > 0.0 ? pow(max(dot(norm, halfDir), 0.0), material.gloss) : 0.0;
    float specStep = step(0.6, spec);

    vec3 texColor = texture(ourTexture, fs_in.TexCoord).rgb;

    vec3 ambient = light.ambient * material.ambient * texColor;
    vec3 diffuse = light.diffuse * material.diffuse * diffStep * texColor;
    vec3 specular = light.specular * material.specular * specStep;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}

