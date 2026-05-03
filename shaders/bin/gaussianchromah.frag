#version 330 core

in vec2 fragUV;

uniform sampler2D imageTex;
uniform float strength;

out vec4 fragColor;

float gaussian(float x, float sigma)
{
    return exp(-(x * x) / (2.0 * sigma * sigma));
}

void main()
{
    ivec2 texSize = textureSize(imageTex, 0);
    ivec2 p = ivec2(fragUV * texSize);
    vec3 center = texelFetch(imageTex, p, 0).rgb;

    float sigma = mix(0.8, 3.0, clamp(strength, 0.0, 1.0));
    int radius = int(ceil(2.0 * sigma));

    vec2 chroma = vec2(0.0);
    float weightSum = 0.0;
    for (int x = -radius; x <= radius; ++x) {
        ivec2 q = clamp(p + ivec2(x, 0), ivec2(0), texSize - ivec2(1));
        vec3 value = texelFetch(imageTex, q, 0).rgb;

        float weight = gaussian(float(x), sigma);
        chroma += value.yz * weight;
        weightSum += weight;
    }

    chroma /= max(weightSum, 1e-6);
    fragColor = vec4(center.x, chroma.x, chroma.y, 1.0);
}