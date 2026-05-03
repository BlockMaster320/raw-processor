#version 330 core

in vec2 fragUV;
uniform sampler2D imageTex;

uniform mat3 rec2020ToSrgb;

out vec4 fragColor;

// Applies ACES tone-mapping.
vec3 tonemapACES(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    ivec2 p = ivec2(fragUV * textureSize(imageTex, 0)); // convert UV to pixel coordinates
    vec3 col = texelFetch(imageTex, p, 0).rgb;

    col = tonemapACES(col);
    col = rec2020ToSrgb * col;  // convert from linear Rec.2020 to linear sRGB for display
    col = clamp(col, 0.0, 1.0);

    fragColor = vec4(col, 1.0);
}