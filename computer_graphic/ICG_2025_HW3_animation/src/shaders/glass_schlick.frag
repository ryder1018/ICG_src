#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform samplerCube skybox;

void main()
{
    float ratio = 1.00 / 1.52;
    vec3 I = normalize(FragPos - viewPos);
    vec3 N = normalize(Normal);

    vec3 R_refract = refract(I, N, ratio);
    vec3 R_reflect = reflect(I, N);

    // Schlick's approximation
    float R0 = pow((1.0 - 1.52) / (1.0 + 1.52), 2.0);
    float cosTheta = dot(-I, N);
    float fresnel = R0 + (1.0 - R0) * pow(1.0 - cosTheta, 5.0);

    vec4 refractColor = texture(skybox, R_refract);
    vec4 reflectColor = texture(skybox, R_reflect);

    FragColor = mix(refractColor, reflectColor, fresnel);
}