#version 330 core

in vec2 fragUV;
uniform sampler2D imageTex;

uniform mat3 rec2020ToSrgb;

out vec4 fragColor;

// ACES tone mapping by Stephen Hill (@self_shadow)
// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
// Note: Matrices are transposed from C++ row-major to GLSL column-major
const mat3 ACESInputMat = mat3(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
// Note: Matrices are transposed from C++ row-major to GLSL column-major
const mat3 ACESOutputMat = mat3(
    1.60475, -0.10208, -0.00327,
    -0.53108, 1.10813, -0.07276,
    -0.07367, -0.00605, 1.07602
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = ACESInputMat * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = ACESOutputMat * color;

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

void main() {
    ivec2 p = ivec2(fragUV * textureSize(imageTex, 0)); // convert UV to pixel coordinates
    vec3 col = texelFetch(imageTex, p, 0).rgb;

    // Convert from Rec.2020 to sRGB primaries first (ACES requires sRGB primaries)
    col = rec2020ToSrgb * col;

    // Apply ACES tone mapping (output is in sRGB)
    col = ACESFitted(col);
    col *= 2.8; // boost brightness for compensate for the tone mapper dimming the image

    fragColor = vec4(col, 1.0);
}