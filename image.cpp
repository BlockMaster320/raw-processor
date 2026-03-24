#include "image.h"

#include "libraw/libraw.h"

#include <algorithm>
#include <utility>

#include <QDebug>

Image::Image(const std::string& path)
	: imagePath(path), rawPixels(), rawWidth(0), rawHeight(0), imageWidth(0), imageHeight(0),
	  leftMargin(0), topMargin(0), blackLevels(), wbMultipliers(), camToSrgbMat(), camToXyzMat(),
	  referencePixels(), referenceWidth(0), referenceHeight(0), thumbnail(), isLoaded(false) {}

bool Image::loadRawData()
{
	qDebug() << "----------------- START -----------------";

	clearLoadedData();
	if (imagePath.empty())
		return false;

	LibRaw rawProcessor;
	int ret = rawProcessor.open_file(imagePath.c_str());
	if (ret != LIBRAW_SUCCESS)
		return false;

	ret = rawProcessor.unpack(); // extract and decode raw image data
	if (ret != LIBRAW_SUCCESS)
		return false;

	const uint16_t* rawData = rawProcessor.imgdata.rawdata.raw_image;
	if (!rawData)
		return false;

	rawWidth = rawProcessor.imgdata.sizes.raw_width;
	rawHeight = rawProcessor.imgdata.sizes.raw_height;
	imageWidth = rawProcessor.imgdata.sizes.width;
	imageHeight = rawProcessor.imgdata.sizes.height;
	leftMargin = rawProcessor.imgdata.sizes.left_margin;
	topMargin = rawProcessor.imgdata.sizes.top_margin;

	rawPixels.assign(rawData, rawData + (rawWidth * rawHeight));

	const uint* cblack = rawProcessor.imgdata.color.cblack; // per-channel black offsets
	const uint black = rawProcessor.imgdata.color.black;    // global black level offset
	const uint white = rawProcessor.imgdata.color.maximum;  // white level (maximum sensor value)

	blackLevels = QVector4D(
		float(cblack[0] + black),
		float(cblack[1] + black),
		float(cblack[2] + black),
		float(cblack[3] + black)
	);

	// Retrieve and normalize WB coefficients
	const float maxMinusBlack = std::max(1.0f, float(white) - float(black));
	float wbMul[4] = {
		rawProcessor.imgdata.color.cam_mul[0],
		rawProcessor.imgdata.color.cam_mul[1],
		rawProcessor.imgdata.color.cam_mul[2],
		rawProcessor.imgdata.color.cam_mul[3]
	};

	const float greenMul = wbMul[1]; // normalize WB coefficients by the green channel value
	for (int c = 0; c < 4; ++c)
		wbMul[c] = (wbMul[c] / greenMul) / maxMinusBlack;

	wbMultipliers = QVector4D(wbMul[0], wbMul[1], wbMul[2], wbMul[3]);

	// Prepare color space conversion matrices
	for (int i = 0; i < 3; i++) {
		camToSrgbMat(i, 0) = rawProcessor.imgdata.color.rgb_cam[i][0];
		camToSrgbMat(i, 1) = rawProcessor.imgdata.color.rgb_cam[i][1] + rawProcessor.imgdata.color.rgb_cam[i][3];   // merge LibRaw's two green channels (G1+G2) into a 3x3 RGB matrix
		camToSrgbMat(i, 2) = rawProcessor.imgdata.color.rgb_cam[i][2];
	}

	for (int i = 0; i < 3; i++) {
		camToXyzMat(i, 0) = rawProcessor.imgdata.color.cam_xyz[i][0];
		camToXyzMat(i, 1) = rawProcessor.imgdata.color.cam_xyz[i][1];
		camToXyzMat(i, 2) = rawProcessor.imgdata.color.cam_xyz[i][2];
	}

	qDebug() << "-------------- RAW IMAGE INFO --------------";
	qDebug() << "CFA pattern: " << rawProcessor.imgdata.idata.cdesc;
	qDebug() << "Raw image dimensions: " << rawWidth << ", " << rawHeight;
	qDebug() << "Real image dimensions: " << imageWidth << ", " << imageHeight;
	qDebug() << "CFA phase offset (left, top):" << leftMargin << "," << topMargin;
	qDebug() << "cblack levels:" << cblack[0] << ", " << cblack[1] << ", " << cblack[2] << ", " << cblack[3];
	qDebug() << "global black level:" << black;
	qDebug() << "white level:" << white;
	qDebug() << "WB multipliers (R,G1,B,G2):"
			 << wbMul[0] << "," << wbMul[1] << ","
			 << wbMul[2] << "," << wbMul[3];

	qDebug() << "----------------- END -----------------";

	isLoaded = true;
	return true;
}

bool Image::loadThumbnail()
{
	thumbnail.clear();
	if (imagePath.empty())
		return false;

	LibRaw thumbProcessor;
	int ret = thumbProcessor.open_file(imagePath.c_str());
	if (ret != LIBRAW_SUCCESS)
		return false;

	ret = thumbProcessor.unpack_thumb();
	if (ret != LIBRAW_SUCCESS)
		return false;

	libraw_processed_image_t* thumb = thumbProcessor.dcraw_make_mem_thumb(&ret);
	if (!thumb || ret != LIBRAW_SUCCESS)
		return false;

	thumbnail.assign(thumb->data, thumb->data + thumb->data_size);
	LibRaw::dcraw_clear_mem(thumb);

	return true;
}

// Builds a reference RGB image using LibRaw's internal processing pipeline (on CPU) with settings chosen to best match the GPU pipeline's output for direct pixel comparison.
bool Image::buildReferenceImage()
{
	referencePixels.clear();
	referenceWidth = 0;
	referenceHeight = 0;

	if (imagePath.empty())
		return false;

	LibRaw referenceProcessor;
	int ret = referenceProcessor.open_file(imagePath.c_str());
	if (ret == LIBRAW_SUCCESS)
		ret = referenceProcessor.unpack();

	if (ret == LIBRAW_SUCCESS) {
		// Match GPU pipeline as closely as LibRaw allows.
		//referenceProcessor.imgdata.params.user_qual = 0;        // bilinear demosaic (lin_interpolate)
		//referenceProcessor.imgdata.params.four_color_rgb = 1;   // enable G1/G2 mixing path so interpolation runs as 3-color
		//referenceProcessor.imgdata.params.use_camera_wb = 0;    // force explicit user WB for deterministic compare
		//referenceProcessor.imgdata.params.use_auto_wb = 0;
		//referenceProcessor.imgdata.params.no_auto_scale = 0;    // keep scale_colors (black/white normalization + WB)
		//referenceProcessor.imgdata.params.no_interpolation = 0;
		//referenceProcessor.imgdata.params.highlight = 0;
		//referenceProcessor.imgdata.params.user_flip = 0;        // keep sensor/native orientation for direct compare

		// Force same WB source as GPU path: cam_mul values from metadata.
		for (int c = 0; c < 4; ++c)
			referenceProcessor.imgdata.params.user_mul[c] = referenceProcessor.imgdata.color.cam_mul[c];

		// Disable tone shaping so reference stays linear.
		/*
		referenceProcessor.imgdata.params.no_auto_bright = 1;
		referenceProcessor.imgdata.params.bright = 1.0f;
		referenceProcessor.imgdata.params.gamm[0] = 1.0;
		referenceProcessor.imgdata.params.gamm[1] = 0.0;
		referenceProcessor.imgdata.params.gamm[2] = 0.0;
		referenceProcessor.imgdata.params.gamm[3] = 0.0;
		referenceProcessor.imgdata.params.gamm[4] = 0.0;
		referenceProcessor.imgdata.params.gamm[5] = 0.0;
		referenceProcessor.imgdata.params.output_bps = 16;
		referenceProcessor.imgdata.params.exp_correc = 0;*/
		ret = referenceProcessor.dcraw_process();
	}

	if (ret != LIBRAW_SUCCESS || !referenceProcessor.imgdata.image)
		return false;

	referenceWidth = referenceProcessor.imgdata.sizes.width;
	referenceHeight = referenceProcessor.imgdata.sizes.height;
	referencePixels.resize(referenceWidth * referenceHeight * 3);

	uint16_t (*img)[4] = referenceProcessor.imgdata.image;
	for (int y = 0; y < referenceHeight; ++y) {
		for (int x = 0; x < referenceWidth; ++x) {
			const int srcIdx = y * referenceWidth + x;
			const int dstIdx = (y * referenceWidth + x) * 3;
			referencePixels[dstIdx + 0] = img[srcIdx][0];
			referencePixels[dstIdx + 1] = img[srcIdx][1];
			referencePixels[dstIdx + 2] = img[srcIdx][2];
		}
	}

	return true;
}

// Getters & setters
bool Image::getIsLoaded() const { return isLoaded; }

void Image::setPath(const std::string& path) { imagePath = path; }
const std::string& Image::getPath() const { return imagePath; }

const uint16_t* Image::getRawData() const{ return rawPixels.empty() ? nullptr : rawPixels.data(); }
int Image::getRawWidth() const { return rawWidth; }
int Image::getRawHeight() const { return rawHeight; }

int Image::getImageWidth() const { return imageWidth; }
int Image::getImageHeight() const { return imageHeight; }
int Image::getLeftMargin() const { return leftMargin; }
int Image::getTopMargin() const { return topMargin; }
const QVector4D& Image::getBlackLevels() const { return blackLevels; }
const QVector4D& Image::getWbMultipliers() const { return wbMultipliers; }
const QMatrix3x3& Image::getCamToSrgb() const { return camToSrgbMat; }
const QMatrix3x3& Image::getCamToXyz() const { return camToXyzMat; }

const uint16_t* Image::getReferenceData() const { return referencePixels.empty() ? nullptr : referencePixels.data(); }

int Image::getReferenceWidth() const { return referenceWidth; }
int Image::getReferenceHeight() const { return referenceHeight; }
const std::vector<unsigned char>& Image::getThumbnailBytes() const { return thumbnail; }

void Image::clearLoadedData()
{
	rawPixels.clear();
	rawWidth = 0;
	rawHeight = 0;
	imageWidth = 0;
	imageHeight = 0;
	leftMargin = 0;
	topMargin = 0;
	blackLevels = QVector4D();
	wbMultipliers = QVector4D();
	camToSrgbMat = QMatrix3x3();
	camToXyzMat = QMatrix3x3();
	isLoaded = false;
}

