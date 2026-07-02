#version 330 core

in vec2 fragUV;
uniform sampler2D imageTex;

out vec4 fragColor;

void main() {
    ivec2 p = ivec2(fragUV * textureSize(imageTex, 0));
    vec3 col = texelFetch(imageTex, p, 0).rgb;

    col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));
    
    fragColor = vec4(col, 1.0);
}
