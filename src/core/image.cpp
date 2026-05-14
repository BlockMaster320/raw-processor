#include "image.h"
#include "core/adjustmentmanager.h"

#include "libraw/libraw.h"
#include "core/utility.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Image::Image(const QString& path)
	: imagePath(path), rawPixels(), rawWidth(0), rawHeight(0), imageWidth(0), imageHeight(0),
	  leftMargin(0), topMargin(0), blackLevels(), wbMultipliers(), camToSrgbMat(), camToXyzMat(),
	  referencePixels(), referenceWidth(0), referenceHeight(0), thumbnail(), isLoaded(false) {}

bool Image::loadRawData()
{
	qDebug() << "----------------- START -----------------";

	clearLoadedData();
	if (imagePath.isEmpty())
		return false;

	LibRaw rawProcessor;
	int ret = rawProcessor.open_file(imagePath.toStdWString().c_str());
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

	// --  Prepare color space conversion matrices --

	// Construct the camera -> Rec.2020 and Rec.2020 -> camera matrices using libraw's
	// camera->sRGB matrix (the only one provided by libraw which seem to have correct information).
	// Sequence of conversions: camera -> sRGB -> XYZ -> Rec.2020 -> sRGB
	for (int i = 0; i < 3; i++) {	// camera -> sRGB matrix
		camToSrgbMat(i, 0) = rawProcessor.imgdata.color.rgb_cam[i][0];
		camToSrgbMat(i, 1) = rawProcessor.imgdata.color.rgb_cam[i][1] + rawProcessor.imgdata.color.rgb_cam[i][3];   // merge LibRaw's two green channels (G1+G2) into a 3x3 RGB matrix
		camToSrgbMat(i, 2) = rawProcessor.imgdata.color.rgb_cam[i][2];
	}

	QMatrix3x3 srgbToXyzMat;	// sRGB -> XYZ matrix (D65)
	srgbToXyzMat(0, 0) = 0.4123908f; srgbToXyzMat(0, 1) = 0.3575843f; srgbToXyzMat(0, 2) = 0.1804808f;
	srgbToXyzMat(1, 0) = 0.2126390f; srgbToXyzMat(1, 1) = 0.7151687f; srgbToXyzMat(1, 2) = 0.0721923f;
	srgbToXyzMat(2, 0) = 0.0193308f; srgbToXyzMat(2, 1) = 0.1191948f; srgbToXyzMat(2, 2) = 0.9505322f;

	QMatrix3x3 xyzToRec2020Mat;	// XYZ -> Rec2020 matrix
	xyzToRec2020Mat(0, 0) = 1.7166512f;  xyzToRec2020Mat(0, 1) = -0.3556708f; xyzToRec2020Mat(0, 2) = -0.2533663f;
	xyzToRec2020Mat(1, 0) = -0.6666844f; xyzToRec2020Mat(1, 1) = 1.6164812f;  xyzToRec2020Mat(1, 2) = 0.0157685f;
	xyzToRec2020Mat(2, 0) = 0.0176399f;  xyzToRec2020Mat(2, 1) = -0.0427706f; xyzToRec2020Mat(2, 2) = 0.9421031f;

	QMatrix3x3 rec2020ToXyzMat = invert3x3(xyzToRec2020Mat);
	QMatrix3x3 xyzToSrgbMat = invert3x3(srgbToXyzMat);

	camToXyzMat = srgbToXyzMat * camToSrgbMat;
	camToRec2020Mat = xyzToRec2020Mat * camToXyzMat;
	rec2020ToSrgbMat = xyzToSrgbMat * rec2020ToXyzMat;

	
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

	qDebug() << "----------------------------------";

	isLoaded = true;
	return true;
}

// Builds a reference RGB image using LibRaw's internal processing pipeline (on CPU) with settings chosen to best match the GPU pipeline's output for direct pixel comparison.
bool Image::buildReferenceImage()
{
	referencePixels.clear();
	referenceWidth = 0;
	referenceHeight = 0;

	if (imagePath.isEmpty())
		return false;

	LibRaw referenceProcessor;
	int ret = referenceProcessor.open_file(imagePath.toStdWString().c_str());
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
const QMatrix3x3& Image::getCamToRec2020() const { return camToRec2020Mat; }
const QMatrix3x3 &Image::getRec2020ToSrgb() const {return rec2020ToSrgbMat; }

const uint16_t* Image::getReferenceData() const { return referencePixels.empty() ? nullptr : referencePixels.data(); }

int Image::getReferenceWidth() const { return referenceWidth; }
int Image::getReferenceHeight() const { return referenceHeight; }

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
	camToRec2020Mat = QMatrix3x3();
	rec2020ToSrgbMat = QMatrix3x3();
	isLoaded = false;
}

QString Image::getSidecarPath() const {
	QFileInfo fileInfo(imagePath);
	QString baseName = fileInfo.baseName();
	QString directory = fileInfo.dir().absolutePath();
	return QDir(directory).filePath(baseName + ".adjustments.json");
}

void Image::loadAdjustmentCells(AdjustmentManager* acm) {
	adjustmentCells.clear();

	// Try to load from sidecar file
	QString sidecarPath = getSidecarPath();
	QFile file(sidecarPath);

	if (file.exists() && file.open(QIODevice::ReadOnly)) {
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		file.close();

		if (doc.isObject()) {
			QJsonObject root = doc.object();
			if (root.contains("cells") && root["cells"].isArray()) {
				QJsonArray cellArray = root["cells"].toArray();

				for (const auto& cellJson : cellArray) {
					if (!cellJson.isObject()) continue;

					QJsonObject cellObj = cellJson.toObject();
					bool visible = cellObj.value("visible").toBool(true);
					bool isLinked = cellObj.value("linked").toBool(false);
					QString dataIdStr = cellObj.value("dataId").toString();
					QUuid dataId = dataIdStr.isEmpty() ? QUuid() : QUuid(dataIdStr);

					AdjustmentCell cell;
					cell.isVisible = visible;
					cell.isCollapsed = cellObj.value("collapsed").toBool(false);

					// Get cell name from JSON (for backward compat with instanceName field)
					QString cellName = cellObj.value("instanceName").toString();
					if (cellName.isEmpty()) {
						cellName = cellObj.value("name").toString("Cell");
					}

					if (isLinked && acm && !dataId.isNull()) {
						// Load linked cell - get data from manager
						acm->refreshCellData(dataId);
						auto cellData = acm->getCellData(dataId);
						if (cellData) {
							cell.data = cellData;
						} else {
							// Fallback: create new cell with cached adjustments from sidecar
							cell.data = std::make_shared<AdjustmentGroup>();
							cell.data->id = dataId;
							cell.data->isGlobal = false;
							
							if (cellObj.contains("adjustments") && cellObj["adjustments"].isObject()) {
								cell.data->fromJson(cellObj);
								cell.data->id = dataId;
							}
							acm->registerCellData(cell.data);
						}
					} else {
						// Load static cell (unlinked)
						cell.data = std::make_shared<AdjustmentGroup>(cellName);

						// Load adjustments from JSON
						if (cellObj.contains("adjustments") && cellObj["adjustments"].isObject()) {
							cell.data->fromJson(cellObj);
							cell.data->name = cellName;
						} else {
							// Initialize with default adjustments
							cell.data->adjustments[AdjType::Denoise]    = std::make_unique<AdjDenoise>();
							cell.data->adjustments[AdjType::Exposure]   = std::make_unique<AdjExposure>();
							cell.data->adjustments[AdjType::Contrast]   = std::make_unique<AdjContrast>();
							cell.data->adjustments[AdjType::Midpoint]   = std::make_unique<AdjMidpoint>();
							cell.data->adjustments[AdjType::PopArt]     = std::make_unique<AdjPopArt>();
							cell.data->adjustments[AdjType::WhiteBlack] = std::make_unique<AdjWhiteBlack>();
							cell.data->adjustments[AdjType::Saturation] = std::make_unique<AdjSaturation>();
						}
					}

					adjustmentCells.push_back(cell);
				}
			}
		}
	}

	// If no cells loaded, create default cell
	if (adjustmentCells.empty()) {
		AdjustmentCell defaultCell("Base");
		adjustmentCells.push_back(defaultCell);
	}
}

void Image::saveAdjustmentCells() {
	QString sidecarPath = getSidecarPath();
	QJsonObject root;
	root["imagePath"] = imagePath;

	QJsonArray cellArray;
	for (const auto& cell : adjustmentCells) {
		QJsonObject cellObj;
		cellObj["visible"] = cell.isVisible;
		cellObj["linked"] = cell.isLinked();
		cellObj["collapsed"] = cell.isCollapsed;

		if (cell.data) {
			cellObj["dataId"] = cell.data->id.toString();
			cellObj["name"] = cell.data->name;

			// Serialize adjustments using the cell data's toJson method
			QJsonObject cellDataJson = cell.data->toJson();
			cellObj["adjustments"] = cellDataJson.value("adjustments").toObject();
		}

		cellArray.append(cellObj);
	}

	root["cells"] = cellArray;

	QFile file(sidecarPath);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Could not save adjustment cells to:" << sidecarPath;
		return;
	}

	QJsonDocument doc(root);
	file.write(doc.toJson());
	file.close();
}



