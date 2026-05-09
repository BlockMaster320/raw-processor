#pragma once

#include "../adjustmentcellmanager.h"

#include <memory>

#include <QWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>

class Image;
class AdjustmentCellData;

// Widget for managing adjustment cell presets and applying cells to images
class AdjustmentCellManagerWidget : public QWidget {
    Q_OBJECT

public:
    explicit AdjustmentCellManagerWidget(std::shared_ptr<AdjustmentCellManager> acm, QWidget* parent = nullptr);

    void updatePresetList();
    void updateActiveCell();
    void clearPresetSelection();

signals:
    void presetActivated();
    void appliedToImages();

private slots:
    void onPresetScopeChanged(int modeId);
    void onPresetSelected(QListWidgetItem* item);
    void onPresetItemChanged(QListWidgetItem* item);
    void onRemovePresetClicked();
    void onApplyClicked();
    void onSaveAsPresetClicked();
    void onApplyModeChanged(int modeId);
    void onEyeButtonToggled();

private:
    void setupUI();
    void setupConnections();
    void populatePresetList();

    std::shared_ptr<AdjustmentCellManager> acm;

    // UI Components
    QWidget* presetScopeSwitchWidget;   // segmented switch container
    QPushButton* localScopeButton;      // local scope segment
    QPushButton* globalScopeButton;     // global scope segment
    QButtonGroup* presetScopeGroup;     // exclusive scope group
    QListWidget* presetList;            // list of presets
    QLabel* activeCellNameLabel;        // name of active cell
    QPushButton* eyeButton;             // show/hide toggle
    QPushButton* applyButton;           // apply to selected images
    QPushButton* saveAsPresetButton;    // save as preset
    QWidget* applyModeSwitchWidget;     // segmented switch container
    QPushButton* copyModeButton;        // copy mode segment
    QPushButton* linkModeButton;        // link mode segment
    QButtonGroup* applyModeGroup;       // exclusive mode group
    QPushButton* removePresetButton;    // remove selected preset

    bool isRefreshingPresetList = false;
};
