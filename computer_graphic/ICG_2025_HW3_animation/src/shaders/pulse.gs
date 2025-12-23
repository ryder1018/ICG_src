#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} gs_in[];

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} gs_out;

uniform float time;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec3 center = (gs_in[0].FragPos + gs_in[1].FragPos + gs_in[2].FragPos) / 3.0;
    float seed = float(gl_PrimitiveIDIn);
    float wobble = sin(time * 3.0 + seed * 0.07);
    float scale = 1.0 + 0.15 * wobble;

    for (int i = 0; i < 3; i++) {
        vec3 dir = gs_in[i].FragPos - center;
        vec3 newPos = center + dir * scale;

        gl_Position = projection * view * vec4(newPos, 1.0);
        gs_out.FragPos = newPos;
        gs_out.Normal = gs_in[i].Normal;
        gs_out.TexCoord = gs_in[i].TexCoord;
        EmitVertex();
    }
    EndPrimitive();
}
