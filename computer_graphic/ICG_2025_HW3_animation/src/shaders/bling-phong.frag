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
    vec3 norm    = normalize(fs_in.Normal);
    vec3 lightDir = normalize(light.position - fs_in.FragPos);
    vec3 viewDir  = normalize(viewPos - fs_in.FragPos);

    // Blinn‑Phong：用 half vector
    vec3 halfDir = normalize(lightDir + viewDir);

    float diff = max(dot(norm, lightDir), 0.0);
    float spec = 0.0;
    if (diff > 0.0)
        spec = pow(max(dot(norm, halfDir), 0.0), material.gloss);

    vec3 texColor = texture(ourTexture, fs_in.TexCoord).rgb;

    vec3 ambient  = light.ambient  * material.ambient  * texColor;
    vec3 diffuse  = light.diffuse  * material.diffuse  * diff * texColor;
    vec3 specular = light.specular * material.specular * spec;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
