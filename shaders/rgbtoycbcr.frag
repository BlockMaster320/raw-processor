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
    vec3 rgb = texelFetch(imageTex, p, 0).rgb;

    // Rec.2020 to YCbCr conversion (source: https://www.itu.int/rec/R-REC-BT.2020)
    float y  = dot(rgb, vec3(R, G, B));
    float cb = (rgb.b - y) / (2.0 * (1.0 - B));
    float cr = (rgb.r - y) / (2.0 * (1.0 - R));

    fragColor = vec4(y, cb, cr, 1.0);
}
