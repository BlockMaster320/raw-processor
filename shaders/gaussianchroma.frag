// Performs a Gaussian blur on the chroma channels (Cb and Cr).
// This is a separable filter, so we run it in two passes: first horizontally, then vertically.

#version 330 core

in vec2 fragUV;

uniform sampler2D imageTex;
uniform float strength;
uniform int horizontal;  // 1 = horizontal pass, 0 = vertical pass

out vec4 fragColor;

float gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

void main()
{
    ivec2 texSize = textureSize(imageTex, 0);
    ivec2 p = ivec2(fragUV * texSize);
    vec3 original = texelFetch(imageTex, p, 0).rgb;

    float sigma = mix(0.8, 6.0, strength);
    int radius = int(ceil(sigma * 2.0));

    vec2 chroma = vec2(0.0);
    float weightSum = 0.0;

    for (int i = -radius; i <= radius; ++i) {
        ivec2 offset = (horizontal == 1) ? ivec2(i, 0) : ivec2(0, i);
        ivec2 q = clamp(p + offset, ivec2(0), texSize - ivec2(1));

        float weight = gaussian(float(i), sigma);
        chroma += texelFetch(imageTex, q, 0).yz * weight;
        weightSum += weight;
    }

    chroma /= max(weightSum, 1e-6);

    fragColor = vec4(original.x, chroma.x, chroma.y, 1.0);
}
