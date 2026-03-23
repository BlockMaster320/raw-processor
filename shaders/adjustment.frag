#version 330 core

in vec2 fragUV;

uniform sampler2D imageTex;
uniform float exposure;

out vec4 fragColor;

void main() {
    ivec2 p = ivec2(fragUV * textureSize(imageTex, 0)); // convert UV to pixel coordinates
    vec3 col = texelFetch(imageTex, p, 0).rgb;

    col *= exposure * 2.;

    fragColor = vec4(col, 1.0);
}