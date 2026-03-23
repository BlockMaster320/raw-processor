#version 330 core

in vec2 fragUV;

uniform sampler2D imageTex;
uniform float exposure;

out vec4 fragColor;

void main() {
    fragColor = texture(imageTex, fragUV) * exposure * 2.;
}