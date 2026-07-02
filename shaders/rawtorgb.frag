#version 330 core

//precision highp float;

in vec2 fragUV;

uniform sampler2D imageTex;

uniform vec4 blackLevels;
uniform vec4 wbMultipliers;
uniform ivec2 cfaOffset;

uniform mat3 camToSRGB;	// color conversion matrices
uniform mat3 camToXYZ;
uniform mat3 camToRec2020;

out vec4 fragColor;

// Determines the color channel of the CFA corresponding to the given pixel position
int getCfaChannel(ivec2 pos) {
	bool evenX = ((pos.x + cfaOffset.x) % 2) == 0;
	bool evenY = ((pos.y + cfaOffset.y) % 2) == 0;

	if (evenY) {
		return evenX ? 0 : 1; // R, G1
	}
	return evenX ? 3 : 2; // G2, B
}

ivec2 clampToImage(ivec2 pos) {
	ivec2 size = textureSize(imageTex, 0);
	return clamp(pos, ivec2(0, 0), size - ivec2(1, 1));
}

// Samples the raw texture at given position and applies black level subtraction and proper normalization to [0, 1] range
float sampleRaw(ivec2 pos) {
	pos = clampToImage(pos);
	float v = texelFetch(imageTex, pos, 0).r * 65535.0;	// shader input values are automatically normalized to [0, 1] range -> for proper normalization first convert back to uint8 range
	int channel = getCfaChannel(pos);

	float lin = max(v - blackLevels[channel], 0.0); // channel black subtraction
	lin *= wbMultipliers[channel];                // multiplication by WB multipliers + proper normalization (WB multipliers are pre-divided by the maximum image value)

	return lin;
}

void main() {
	vec2 fragUVFlipped = vec2(fragUV.x, 1.0 - fragUV.y); // flip Y to match image coordinates
	ivec2 pos = ivec2(fragUVFlipped * textureSize(imageTex, 0)); // convert UV to pixel coordinates

	int channel = getCfaChannel(pos);
	float r, g, b;
	if (channel == 0) {   // red pixel
		r = sampleRaw(pos);
		g = (sampleRaw(pos + ivec2(-1, 0)) +
			 sampleRaw(pos + ivec2(1, 0)) +
			 sampleRaw(pos + ivec2(0, -1)) +
			 sampleRaw(pos + ivec2(0, 1)))
			 / 4.0;
			
		b = (sampleRaw(pos + ivec2(-1, -1)) +
			 sampleRaw(pos + ivec2(1, -1)) +
			 sampleRaw(pos + ivec2(-1, 1)) +
			 sampleRaw(pos + ivec2(1, 1)))
			 / 4.0;
	} else if (channel == 2) {   // blue pixel
		b = sampleRaw(pos);
		g = (sampleRaw(pos + ivec2(-1, 0)) +
			 sampleRaw(pos + ivec2(1, 0)) +
			 sampleRaw(pos + ivec2(0, -1)) +
			 sampleRaw(pos + ivec2(0, 1)))
			 / 4.0;
			
		r = (sampleRaw(pos + ivec2(-1, -1)) +
			 sampleRaw(pos + ivec2(1, -1)) +
			 sampleRaw(pos + ivec2(-1, 1)) +
			 sampleRaw(pos + ivec2(1, 1)))
			 / 4.0;

	} else {    // green pixel
		g = sampleRaw(pos);
		if (channel == 1) {   // green pixel on red row
			r = (sampleRaw(pos + ivec2(-1, 0)) +
				 sampleRaw(pos + ivec2(1, 0)))
				 / 2.0;
			b = (sampleRaw(pos + ivec2(0, -1)) +
				 sampleRaw(pos + ivec2(0, 1)))
				 / 2.0;
		} else {			  // green pixel on blue row
			r = (sampleRaw(pos + ivec2(0, -1)) +
				 sampleRaw(pos + ivec2(0, 1)))
				 / 2.0;
			b = (sampleRaw(pos + ivec2(-1, 0)) +
				 sampleRaw(pos + ivec2(1, 0)))
				 / 2.0;
		}
	}

	vec3 col = vec3(r, g, b);

	// Convert from camera RGB to sRGB
	//col = camToSRGB * col;

    // --- Camera to XYZ conversion ---
    //vec3 xyz = camToXYZ * col;

    // --- XYZ to sRGB conversion ---
    const mat3 XYZtoSRGB = mat3(
        3.2406, -1.5372, -0.4986,
       -0.9689,  1.8758,  0.0415,
        0.0557, -0.2040,  1.0570
    );
    //col = XYZtoSRGB * xyz;

	// --- Camera to Rec2020 conversion ---
	col = camToRec2020 * col;

	fragColor = vec4(max(col, 0.0), 1.0);
}