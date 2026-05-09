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
    void onEnabledStateChanged(int stateId);
    void onAutoPresetSwitchChanged(int stateId);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUI();
    void setupConnections();
    void populatePresetList();

    std::shared_ptr<AdjustmentCellManager> acm;
    QString previouslySelectedPresetId;  // track preset selection across scope changes

    // UI Components
    QWidget* presetScopeSwitchWidget;   // segmented switch container
    QPushButton* localScopeButton;      // local scope segment
    QPushButton* globalScopeButton;     // global scope segment
    QButtonGroup* presetScopeGroup;     // exclusive scope group
    QListWidget* presetList;            // list of presets
    QLabel* presetsLabel;               // label for presets
    QWidget* activeCellControlsWidget;  // container for active cell controls (shown/hidden based on active cell)
    QLabel* activeCellNameLabel;        // shows name of active cell or preset
    QWidget* enabledStateWidget;        // segmented switch container
    QPushButton* enabledButton;         // enabled state segment
    QPushButton* disabledButton;        // disabled state segment
    QButtonGroup* enabledStateGroup;    // exclusive state group
    QLabel* autoPresetLabel;            // auto preset option label
    QWidget* autoPresetSwitchWidget;    // compact switch container
    QPushButton* autoPresetOffButton;   // compact switch off segment
    QPushButton* autoPresetOnButton;    // compact switch on segment
    QButtonGroup* autoPresetSwitchGroup;// compact switch group
    QPushButton* applyButton;           // apply to selected images
    QPushButton* saveAsPresetButton;    // save as preset
    QWidget* applyModeSwitchWidget;     // segmented switch container
    QPushButton* copyModeButton;        // copy mode segment
    QPushButton* linkModeButton;        // link mode segment
    QButtonGroup* applyModeGroup;       // exclusive mode group
    QPushButton* removePresetButton;    // remove selected preset

    bool isRefreshingPresetList = false;
};
