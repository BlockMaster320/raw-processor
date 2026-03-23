#version 330 core

in vec2 fragUV;

uniform sampler2D imageTex;
uniform sampler2D referenceTex;
uniform int compareMode;

out vec4 fragColor;

vec3 sampleRef(vec2 uv) {
    return texture(referenceTex, vec2(uv.x, 1.0 - uv.y)).rgb;
}

void main() {
    vec3 gpu = texture(imageTex, fragUV).rgb;
    vec3 ref = sampleRef(fragUV);

    if (compareMode == 1) {
       if (fragUV.x < 0.5) {
           vec2 uv = vec2(fragUV.x * 2.0, fragUV.y);
           fragColor = texture(imageTex, uv);
        } else {
            vec2 uv = vec2((fragUV.x - 0.5) * 2.0, fragUV.y);
            fragColor = vec4(sampleRef(uv), 1.0);
        }
    } else if (compareMode == 2) {
        // Absolute difference heatmap: brighter means larger mismatch
        vec3 d = abs(gpu - ref);
        float e = max(max(d.r, d.g), d.b);
        float gain = 8.0;
        fragColor = vec4(vec3(clamp(e * gain, 0.0, 1.0)), 1.0);
    } else if (compareMode == 3) {
        // Signed per-channel difference centered at 0.5 gray
        vec3 diff = (gpu - ref) * 6.0;
        fragColor = vec4(clamp(0.5 + diff, 0.0, 1.0), 1.0);
    } else {
        fragColor = vec4(gpu, 1.0);
    }

    fragColor = pow(fragColor, vec4(1.0/2.2)); // gamma correction

}