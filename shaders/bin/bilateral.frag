#version 330 core

in vec2 fragUV;

uniform sampler2D imageTex;
uniform float strength;

const vec3 REC2020_LUMA = vec3(0.2627, 0.6780, 0.0593);
const int size = 4; // kernel radius
const float intensitySigma = 0.1; // controls how much intensity differences affect the weight
const float spatialSigma = 2.0;   // controls how much spatial distance affects the weight

out vec4 fragColor;

void main()
{
    ivec2 texSize = textureSize(imageTex, 0);
    ivec2 p = ivec2(fragUV * texSize);

    vec3 original = texelFetch(imageTex, p, 0).rgb;
    float originalLum = dot(original, REC2020_LUMA);
    
    vec3 denoised = vec3(0.0);
    float weightSum = 0.0;
    
    for (int y = -size; y <= size; ++y) {
        for (int x = -size; x <= size; ++x) {
            ivec2 q = clamp(p + ivec2(x, y), ivec2(0), texSize - ivec2(1));
            vec3 val = texelFetch(imageTex, q, 0).rgb;

            float spatialWeight = exp(-(x*x + y*y) / (2.0 * spatialSigma * spatialSigma));

            float valLum = dot(val, REC2020_LUMA);
            float diff = valLum - originalLum;  // intensity difference based on luma
            float intensityWeight = exp(-(diff * diff) / (2.0 * intensitySigma * intensitySigma));

            float weight = spatialWeight * intensityWeight;

            denoised += val * weight;
            weightSum += weight;
        }
    }

    denoised /= max(weightSum, 1e-6);   // avoid division by zero

    // Simple 3x3 box blur.
    /*vec3 accum = vec3(0.0);
    accum += texelFetch(imageTex, p + ivec2(-1, -1), 0).rgb * 1.0;
    accum += texelFetch(imageTex, p + ivec2( 0, -1), 0).rgb * 2.0;
    accum += texelFetch(imageTex, p + ivec2( 1, -1), 0).rgb * 1.0;
    accum += texelFetch(imageTex, p + ivec2(-1,  0), 0).rgb * 2.0;
    accum += texelFetch(imageTex, p + ivec2( 0,  0), 0).rgb * 4.0;
    accum += texelFetch(imageTex, p + ivec2( 1,  0), 0).rgb * 2.0;
    accum += texelFetch(imageTex, p + ivec2(-1,  1), 0).rgb * 1.0;
    accum += texelFetch(imageTex, p + ivec2( 0,  1), 0).rgb * 2.0;
    accum += texelFetch(imageTex, p + ivec2( 1,  1), 0).rgb * 1.0;

    vec3 original = texelFetch(imageTex, p, 0).rgb;
    vec3 denoised = accum / 16.0;*/

    fragColor = vec4(mix(original, denoised, strength), 1.0);

    //fragColor = vec4(denoised, 1.0);
}
