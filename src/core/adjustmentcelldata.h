#pragma once

#include "adjustment.h"

#include <memory>
#include <vector>
#include <unordered_map>

#include <QString>
#include <QUuid>
#include <QJsonObject>

// Represents the shared data of a dynamically linked adjustment cell.
// Multiple AdjustmentCell instances can point to the same AdjustmentCellData
// to create dynamic linking - changes to this data affect all linked cells.
class AdjustmentCellData {
public:
    AdjustmentCellData();
    explicit AdjustmentCellData(const QString& cellName);

    QUuid id;  // unique identifier; QUuid() if not dynamically linked
    QString name;
    bool isEnabled = true;
    bool isGlobal = false;

    std::unordered_map<AdjType, std::unique_ptr<Adjustment>> adjustments;

    // Check if this cell data is linked (has a non-null ID)
    bool isLinked() const;

    // Create a deep copy of all adjustments
    std::unordered_map<AdjType, std::unique_ptr<Adjustment>> cloneAdjustments() const;

    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

    // Create a default/base cell data
    static std::shared_ptr<AdjustmentCellData> createDefault();
};
