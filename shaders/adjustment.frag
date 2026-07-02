#version 330 core

in vec2 fragUV;

uniform sampler2D imageTex;
uniform float temperature; // warm/cool balance: -1 cools, +1 warms
uniform float tint;        // green/magenta balance: -1 green, +1 magenta
uniform float exposure;  // EV stops: 0 = neutral, +1 = one stop brighter, -1 = one stop darker
uniform float contrast;  // 1 = neutral, <1 = less contrast, >1 = more contrast
uniform float midpoint;  // 0 = black, 1 = white, 0.5 = neutral
uniform float popArt;
uniform float white;
uniform float black;
uniform float saturation;
uniform float vignette;

const vec3 REC2020_LUMA = vec3(0.2627, 0.6780, 0.0593);

out vec4 fragColor;

vec3 applyExposure(vec3 col, float ev) {
    if (abs(ev) < 1e-6) return col;
    return col * pow(2.0, ev);
}

vec3 applyWhiteBalance(vec3 col, float temperature, float tint) {
    if (abs(temperature) < 1e-6 && abs(tint) < 1e-6) return col;

    // float rMul = 1.0 + (0.25 * temperature) + (0.05 * tint);
    // float bMul = 1.0 - (0.25 * temperature) + (0.05 * tint);
    // float gMul = 1.0 - (0.10 * tint);

    float rMul = 1.0 + (0.5 * temperature) + (0.1 * tint);
    float bMul = 1.0 - (0.5 * temperature) + (0.1 * tint);
    float gMul = 1.0 - (0.25 * tint);

    return max(col * vec3(rMul, gMul, bMul), vec3(0.0));
}

vec3 applyContrast(vec3 col, float contrast, float midpoint) {
    if (abs(contrast) < 1e-6) return col;
    float con = contrast + 1.0; // convert from [-1, 1] to [0, 2] range

    // Simple midpoint contrast
    col = max(col, vec3(0.0));        // keep pow domain valid in linear space
    col = pow(col, vec3(1.0/2.2));    // to gamma space
    col = (col - midpoint) * con + midpoint;
    col = max(col, vec3(0.0));        // avoid pow of negative values after contrast pivoting
    col = pow(col, vec3(2.2));        // back to linear space

    // Luma-based contrast adjustment (preserves hue)
    // vec3 col_gamma = pow(col, vec3(1.0/2.2));   // to gamma space
    // float luma = dot(col_gamma, vec3(0.2126, 0.7152, 0.0722));
    // float newLuma = (luma - 0.5) * con + 0.5;
    // col_gamma *= newLuma / max(luma, 1e-5);
    // col = pow(col_gamma, vec3(2.2));    // back to linear space

    // Sigmoid contrast adjustment (S-curve)
    // col = pow(col, vec3(1.0/2.2));   // to gamma space
    // c = con  * 3.;
    // col = 1.0 / (1.0 + exp(-c * (col - 0.5)));
    // col = pow(col, vec3(2.2));    // back to linear space

    return col;
}

vec3 applyPopArt(vec3 col, float popArt) {
    if (abs(popArt) < 1e-6) return col;
    // ChatGPT's "Lightroom-like" contrast
    float pop = popArt + 1.;    // convert to [0, 2] range
    float pivot = 0.5;
    // 1. core contrast
    col = (col - pivot) * pop + pivot;
    // 2. soft highlight rolloff
    col = col / (col + 1.0);
    // 3. optional saturation compensation
    float luma = dot(col, REC2020_LUMA);
    float sat = 1.0 + 0.2 * (pop - 1.0); // tweak this
    col = mix(vec3(luma), col, sat);

    return col;
}

vec3 applyWhiteBlackLevels(vec3 col, float white, float black) {
    if (abs(white) < 1e-6 && abs(black) < 1e-6) return col;
    float w = white + 1.; // convert from [-1, 1] to [0, 2] range
    float b = black;
    //float range = max(w - b, 1e-5);
    //col = (col - vec3(b)) / range;
    //col /= w;

    col = pow(col, vec3(1.)) * (w - b) + vec3(b);
    col = max(col, vec3(0.));

    return col;
}

vec3 applySaturation(vec3 col, float saturation) {
    if (abs(saturation) < 1e-6) return col;
    float sat = saturation + 1.; // convert to [0, 1] range
    float luma = dot(col, REC2020_LUMA);
    return mix(vec3(luma), col, sat);
}

vec3 applyVignette(vec3 col, vec2 uv, float vignette) {
    if (abs(vignette) < 1e-6) return col;

    vec2 centered = uv * 2. - 1.;
    float d = length(centered);
    float intensity = smoothstep(0.15, 1., d);
    intensity *= intensity;

    float ev = vignette * intensity;
    return col * pow(2.0, ev);
}


void main() {
    ivec2 p = ivec2(fragUV * textureSize(imageTex, 0)); // convert UV to pixel coordinates
    vec3 col = texelFetch(imageTex, p, 0).rgb;

    col = applyWhiteBalance(col, temperature, tint);
    col = applyExposure(col, exposure);
    col = applyContrast(col, contrast, midpoint);
    col = applyWhiteBlackLevels(col, white, black);
    col = applyPopArt(col, popArt);
    col = applySaturation(col, saturation);
    col = applyVignette(col, fragUV, vignette);

    fragColor = vec4(col, 1.0);
}