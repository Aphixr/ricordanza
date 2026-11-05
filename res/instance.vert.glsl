#version 330 core

layout (location = 0) in vec2 a_pos;
layout (location = 1) in mat4 a_model;

uniform mat4 projection;

void main()
{
    gl_Position = projection * a_model * vec4(a_pos, 0.0, 1.0);
}
