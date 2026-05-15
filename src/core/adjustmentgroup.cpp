#include "adjustmentgroup.h"

#include <QJsonObject>
#include <QJsonArray>

AdjustmentGroup::AdjustmentGroup()
    : id(QUuid()), name("Unnamed"), isEnabled(true), isGlobal(false) {}

AdjustmentGroup::AdjustmentGroup(const QString& cellName)
    : id(QUuid()), name(cellName), isEnabled(true), isGlobal(false) {}

bool AdjustmentGroup::isLinked() const {
    return !id.isNull();
}

// Creates a deep copy of all the adjustments.
std::unordered_map<AdjType, std::unique_ptr<Adjustment>> AdjustmentGroup::cloneAdjustments() const {
    std::unordered_map<AdjType, std::unique_ptr<Adjustment>> cloned;
    for (const auto& [type, adj] : adjustments) {
        if (adj) {
            cloned[type] = adj->clone();
        }
    }
    return cloned;
}

QJsonObject AdjustmentGroup::toJson() const {
    QJsonObject obj;
    obj["id"] = id.toString();
    obj["name"] = name;
    obj["enabled"] = isEnabled;
    obj["isGlobal"] = isGlobal;

    // Serialize only adjustment values
    QJsonObject adjObj;
    for (const auto& [type, adj] : adjustments) {
        if (!adj || adj->attributes.empty()) continue;

        QString typeName;

        // Map AdjType to string name
        switch (type) {
            case AdjType::WhiteBalance: typeName = "WhiteBalance"; break;
            case AdjType::Exposure: typeName = "Exposure"; break;
            case AdjType::Contrast: typeName = "Contrast"; break;
            case AdjType::Midpoint: typeName = "Midpoint"; break;
            case AdjType::PopArt: typeName = "PopArt"; break;
            case AdjType::WhiteBlack: typeName = "WhiteBlack"; break;
            case AdjType::Saturation: typeName = "Saturation"; break;
            case AdjType::Vignette: typeName = "Vignette"; break;
            case AdjType::Denoise: typeName = "Denoise"; break;
        }

        // Store only the attribute values
        QJsonArray valueArray;
        for (const auto& attr : adj->attributes) {
            valueArray.append(attr.value);
        }
        adjObj[typeName] = valueArray;
    }
    obj["adjustments"] = adjObj;

    return obj;
}

void AdjustmentGroup::fromJson(const QJsonObject& obj) {
    // Only update ID if present in JSON (preserves existing ID when loading partial JSON)
    if (obj.contains("id")) {
        id = QUuid(obj.value("id").toString());
    }
    name = obj.value("name").toString("Unnamed");
    isEnabled = obj.value("enabled").toBool(true);
    isGlobal = obj.value("isGlobal").toBool(false);

    // Deserialize adjustment values
    if (obj.contains("adjustments") && obj["adjustments"].isObject()) {
        QJsonObject adjObj = obj["adjustments"].toObject();

        // Helper lambda to deserialize adjustments by type
        auto deserializeAdj = [this, &adjObj](const QString& typeName, AdjType type, Adjustment* templateAdj) {
            if (!adjObj.contains(typeName)) return;

            QJsonArray valueArray = adjObj[typeName].toArray();
            if (valueArray.isEmpty()) return;

            auto adj = templateAdj->clone();

            // Restore values from JSON
            for (int i = 0; i < valueArray.size() && i < static_cast<int>(adj->attributes.size()); ++i) {
                adj->attributes[i].value = valueArray[i].toDouble(adj->attributes[i].value);
            }

            adjustments[type] = std::move(adj);
        };

        // Deserialize each adjustment type using templates
        AdjWhiteBalance whiteBalance;
        deserializeAdj("WhiteBalance", AdjType::WhiteBalance, &whiteBalance);

        AdjExposure exposure;
        deserializeAdj("Exposure", AdjType::Exposure, &exposure);

        AdjContrast contrast;
        deserializeAdj("Contrast", AdjType::Contrast, &contrast);

        AdjMidpoint midpoint;
        deserializeAdj("Midpoint", AdjType::Midpoint, &midpoint);

        AdjPopArt popArt;
        deserializeAdj("PopArt", AdjType::PopArt, &popArt);

        AdjWhiteBlack whiteBlack;
        deserializeAdj("WhiteBlack", AdjType::WhiteBlack, &whiteBlack);

        AdjSaturation saturation;
        deserializeAdj("Saturation", AdjType::Saturation, &saturation);

        AdjVignette vignette;
        deserializeAdj("Vignette", AdjType::Vignette, &vignette);

        AdjDenoise denoise;
        deserializeAdj("Denoise", AdjType::Denoise, &denoise);

        // Backward compatibility for sidecars/presets saved before WhiteBalance existed.
        if (adjustments.find(AdjType::WhiteBalance) == adjustments.end()) {
            adjustments[AdjType::WhiteBalance] = std::make_unique<AdjWhiteBalance>();
        }
        if (adjustments.find(AdjType::Vignette) == adjustments.end()) {
            adjustments[AdjType::Vignette] = std::make_unique<AdjVignette>();
        }
    }
}

std::shared_ptr<AdjustmentGroup> AdjustmentGroup::createDefault() {
    auto data = std::make_shared<AdjustmentGroup>("Default");
    data->isEnabled = true;
    return data;
}