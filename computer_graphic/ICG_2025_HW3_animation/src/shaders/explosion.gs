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

uniform float magnitude; // Explosion magnitude (time)
uniform mat4 view;
uniform mat4 projection;

vec3 GetNormal()
{
    // Calculate normal in World Space using FragPos
    vec3 a = gs_in[0].FragPos - gs_in[1].FragPos;
    vec3 b = gs_in[2].FragPos - gs_in[1].FragPos;
    return normalize(cross(a, b));
}

void main() {    
    vec3 offset = vec3(0.0);
    
    // Optimization & Safety: 
    // If magnitude is 0 (Flair), skip normal calculation to avoid NaNs from degenerate triangles.
    if (magnitude != 0.0) {
        vec3 normal = GetNormal();
        // Negative factor because user requested implode/reverse direction? 
        // Or user set -1.0 manually in Step 291. keeping current logic.
        offset = normal * magnitude * 20.0 * (-1.0);
    }

    for(int i = 0; i < 3; i++) {
        vec3 newPos = gs_in[i].FragPos + offset;
        gl_Position = projection * view * vec4(newPos, 1.0);
        
        // Pass to fragment shader
        gs_out.FragPos = newPos;
        gs_out.Normal = gs_in[i].Normal; 
        gs_out.TexCoord = gs_in[i].TexCoord;
        
        EmitVertex();
    }
    EndPrimitive();
}
