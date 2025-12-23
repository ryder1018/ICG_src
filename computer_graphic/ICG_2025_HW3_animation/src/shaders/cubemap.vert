#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aPos;
    mat4 viewNoTrans = mat4(mat3(view));          // 去掉平移，只保留旋轉
    vec4 pos = projection * viewNoTrans * vec4(aPos, 1.0);
    gl_Position = pos.xyww;                       // z = w，永遠畫在最遠處
}
