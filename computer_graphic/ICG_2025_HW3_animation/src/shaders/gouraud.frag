#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 Color;
    vec2 TexCoord;
} fs_in;

uniform sampler2D ourTexture;

void main()
{
    vec3 texColor = texture(ourTexture, fs_in.TexCoord).rgb;
    vec3 result   = texColor * fs_in.Color;
    FragColor = vec4(result, 1.0);
}
