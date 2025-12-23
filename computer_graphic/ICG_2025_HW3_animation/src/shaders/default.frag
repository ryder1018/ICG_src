#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} fs_in; 

uniform sampler2D ourTexture;
uniform vec3 rainbowColor;

void main()
{
    FragColor = texture(ourTexture, fs_in.TexCoord);
} 