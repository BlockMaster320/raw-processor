#version 330 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;

uniform mat4 transform;

out vec2 fragUV;

void main()
{
    fragUV = uv;
    gl_Position = transform * vec4(pos, 0., 1.0);
}