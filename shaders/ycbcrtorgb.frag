#version 330 core

in vec2 fragUV;

uniform sampler2D imageTex;

const float R = 0.2627; // Rec.2020 luma coefficients
const float G = 0.6780;
const float B = 0.0593;

out vec4 fragColor;

void main()
{
    ivec2 p = ivec2(fragUV * textureSize(imageTex, 0));
    vec3 ycbcr = texelFetch(imageTex, p, 0).rgb;

    // YCbCr to Rec.2020 conversion (source: https://www.itu.int/rec/R-REC-BT.2020)
    float y  = ycbcr.x;
    float cb = ycbcr.y;
    float cr = ycbcr.z;

    float r = y + 2.0 * (1.0 - R) * cr;
    float b = y + 2.0 * (1.0 - B) * cb;
    float g = y - (2.0 * B * (1.0 - B) / G) * cb - (2.0 * R * (1.0 - R) / G) * cr;

    fragColor = vec4(max(vec3(r, g, b), vec3(0.0)), 1.0);
}
