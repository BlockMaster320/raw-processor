// Performs a Bilateral blur on the luma channel (Y).

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
    vec3 original = texelFetch(imageTex, p, 0).rgb;

    float spatialSigma = mix(0.4, 2.0, strength) * 0.5;
    float intensitySigma = mix(0.02, 0.1, strength) * 0.5;
    int radius = int(ceil(spatialSigma * 2.0));

    float filteredY = 0.0;
    float weightSum = 0.0;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            ivec2 q = clamp(p + ivec2(x, y), ivec2(0), texSize - ivec2(1));
            vec3 sampleValue = texelFetch(imageTex, q, 0).rgb;

            float spatialWeight = gaussian(length(vec2(x, y)), spatialSigma);
            float intensityWeight = gaussian(sampleValue.x - original.x, intensitySigma);
            float weight = spatialWeight * intensityWeight;

            filteredY += sampleValue.x * weight;
            weightSum += weight;
        }
    }

    filteredY /= max(weightSum, 1e-6);

    fragColor = vec4(filteredY, original.y, original.z, 1.0);
}