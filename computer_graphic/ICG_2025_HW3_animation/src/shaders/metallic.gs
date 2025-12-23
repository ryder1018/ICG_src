#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 6) out;

// Data from vertex shader
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} gs_in[];

// Data to fragment shader
out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} gs_out;

out float isAura;

uniform float time;
uniform mat4 view;
uniform mat4 projection;
uniform float magnitude;
uniform vec3 viewPos; 

// Face normal computed in world space
vec3 GetFaceNormal() {
    vec3 a = gs_in[1].FragPos - gs_in[0].FragPos;
    vec3 b = gs_in[2].FragPos - gs_in[0].FragPos;
    return normalize(cross(a, b));
}

void main() {
    vec3 faceNormal = GetFaceNormal();
    
    // Calculate Explosion Offset for Base Geometry
    vec3 baseExplosionOffset = faceNormal * magnitude * 20.0; 

    // Base geometry (metal surface)
    isAura = 0.0;
    for(int i = 0; i < 3; i++) {
        vec3 explodedPos = gs_in[i].FragPos + baseExplosionOffset;
        
        gs_out.FragPos = explodedPos;
        gs_out.Normal = gs_in[i].Normal; 
        gs_out.TexCoord = gs_in[i].TexCoord;
        
        gl_Position = projection * view * vec4(explodedPos, 1.0);
        EmitVertex();
    }
    EndPrimitive();

    // Aura Effect - Make it explode faster/further!
    isAura = 1.0;
    float auraOffsetDist = 0.5 + sin(time * 3.0) * 1.0; 
    
    // Aura explodes 1.5x further than base
    vec3 auraExplosionOffset = baseExplosionOffset * 1.5; 

    for(int i = 0; i < 3; i++) {
        vec3 explodedPos = gs_in[i].FragPos + auraExplosionOffset;
        vec3 finalPos = explodedPos + faceNormal * auraOffsetDist;

        gs_out.FragPos = finalPos;
        gs_out.Normal = faceNormal;
        gs_out.TexCoord = gs_in[i].TexCoord;

        gl_Position = projection * view * vec4(finalPos, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}
