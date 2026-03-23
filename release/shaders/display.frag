#version 330 core

in vec2 fragUV;

uniform sampler2D imageTex;

out vec4 fragColor;

void main() {
    fragColor = texture(imageTex, fragUV);
    // fragColor = vec4(1.0);
}