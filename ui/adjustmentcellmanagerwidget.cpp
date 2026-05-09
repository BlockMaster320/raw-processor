#include "adjustmentcellmanagerwidget.h"
#include "uiconstants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QLabel>
#include <QDebug>
#include <QSignalBlocker>
#include <QButtonGroup>

namespace {
std::shared_ptr<Preset> findPresetById(std::shared_ptr<AdjustmentCellManager> acm, const QString& idStr) {
    const QUuid id = QUuid(idStr);
    if (id.isNull()) {
        return nullptr;
    }

    const auto& localPresets = acm->getLocalPresets();
    for (const auto& preset : localPresets) {
        if (preset && preset->dataId == id) {
            return preset;
        }
    }

    const auto& globalPresets = acm->getGlobalPresets();
    for (const auto& preset : globalPresets) {
        if (preset && preset->dataId == id) {
            return preset;
        }
    }

    return nullptr;
}
}

AdjustmentCellManagerWidget::AdjustmentCellManagerWidget(std::shared_ptr<AdjustmentCellManager> acm, QWidget* parent)
    : QWidget(parent), acm(acm) {
    setupUI();
    setupConnections();
    updatePresetList();
}

void AdjustmentCellManagerWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Preset scope switch (Local/Global)
    presetScopeSwitchWidget = new QWidget(this);
    auto* presetScopeLayout = new QHBoxLayout(presetScopeSwitchWidget);
    presetScopeLayout->setContentsMargins(4, 4, 4, 4);
    presetScopeLayout->setSpacing(0);

    localScopeButton = new QPushButton("Local", presetScopeSwitchWidget);
    globalScopeButton = new QPushButton("Global", presetScopeSwitchWidget);
    localScopeButton->setCheckable(true);
    globalScopeButton->setCheckable(true);

    presetScopeGroup = new QButtonGroup(presetScopeSwitchWidget);
    presetScopeGroup->setExclusive(true);
    presetScopeGroup->addButton(localScopeButton, 0);
    presetScopeGroup->addButton(globalScopeButton, 1);

    if (acm->getPresetListMode() == PresetScope::Global) {
        globalScopeButton->setChecked(true);
    } else {
        localScopeButton->setChecked(true);
    }

    presetScopeLayout->addWidget(localScopeButton);
    presetScopeLayout->addWidget(globalScopeButton);
    presetScopeSwitchWidget->setStyleSheet(switchStyle);
    mainLayout->addWidget(presetScopeSwitchWidget);

    // Preset list
    presetList = new QListWidget(this);
    presetList->setMaximumHeight(150);
    presetList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    mainLayout->addWidget(presetList);

    // Remove preset button
    removePresetButton = new QPushButton("Remove Selected Preset", this);
    removePresetButton->setStyleSheet(baseButtonStyle);
    mainLayout->addWidget(removePresetButton);

    // Divider
    mainLayout->addSpacing(10);

    // Active cell context menu
    activeCellNameLabel = new QLabel("No cell selected", this);
    mainLayout->addWidget(activeCellNameLabel);

    // Eye button (show/hide)
    QHBoxLayout* eyeButtonLayout = new QHBoxLayout();
    eyeButton = new QPushButton("👁 Visible", this);
    eyeButton->setCheckable(true);
    eyeButton->setChecked(true);
    eyeButton->setMaximumWidth(120);
    eyeButton->setStyleSheet(baseButtonStyle);
    eyeButtonLayout->addWidget(eyeButton);
    eyeButtonLayout->addStretch();
    mainLayout->addLayout(eyeButtonLayout);

    // Apply button with mode
    QHBoxLayout* applyLayout = new QHBoxLayout();
    applyButton = new QPushButton("Apply", this);
    applyButton->setStyleSheet(baseButtonStyle);
    applyModeSwitchWidget = new QWidget(this);
    auto* modeSwitchLayout = new QHBoxLayout(applyModeSwitchWidget);
    modeSwitchLayout->setContentsMargins(4, 4, 4, 4);
    modeSwitchLayout->setSpacing(0);

    copyModeButton = new QPushButton("Copy", applyModeSwitchWidget);
    linkModeButton = new QPushButton("Link", applyModeSwitchWidget);
    copyModeButton->setCheckable(true);
    linkModeButton->setCheckable(true);

    applyModeGroup = new QButtonGroup(applyModeSwitchWidget);
    applyModeGroup->setExclusive(true);
    applyModeGroup->addButton(copyModeButton, 0);
    applyModeGroup->addButton(linkModeButton, 1);

    if (acm->getApplyMode() == AdjustmentCellManager::ApplyMode::Link) {
        linkModeButton->setChecked(true);
    } else {
        copyModeButton->setChecked(true);
    }

    modeSwitchLayout->addWidget(copyModeButton);
    modeSwitchLayout->addWidget(linkModeButton);

    applyModeSwitchWidget->setStyleSheet(switchStyle);

    applyLayout->addWidget(applyButton);
    applyLayout->addWidget(applyModeSwitchWidget);
    applyLayout->addStretch();
    mainLayout->addLayout(applyLayout);

    // Save as preset button with mode
    QHBoxLayout* saveLayout = new QHBoxLayout();
    saveAsPresetButton = new QPushButton("Save as Preset", this);
    saveAsPresetButton->setStyleSheet(baseButtonStyle);
    saveLayout->addWidget(saveAsPresetButton);
    saveLayout->addStretch();
    mainLayout->addLayout(saveLayout);

    mainLayout->addStretch();

    setLayout(mainLayout);
}

void AdjustmentCellManagerWidget::setupConnections() {
    connect(presetScopeGroup, &QButtonGroup::idClicked, this, &AdjustmentCellManagerWidget::onPresetScopeChanged);
    connect(presetList, &QListWidget::itemSelectionChanged, this, [this]() {
        if (auto item = presetList->currentItem()) {
            onPresetSelected(item);
        }
    });
    connect(presetList, &QListWidget::itemClicked, this, &AdjustmentCellManagerWidget::onPresetSelected);
    connect(removePresetButton, &QPushButton::clicked, this, &AdjustmentCellManagerWidget::onRemovePresetClicked);
    connect(presetList, &QListWidget::itemChanged, this, &AdjustmentCellManagerWidget::onPresetItemChanged);
    connect(applyButton, &QPushButton::clicked, this, &AdjustmentCellManagerWidget::onApplyClicked);
    connect(saveAsPresetButton, &QPushButton::clicked, this, &AdjustmentCellManagerWidget::onSaveAsPresetClicked);
    connect(applyModeGroup, &QButtonGroup::idClicked, this, &AdjustmentCellManagerWidget::onApplyModeChanged);
    connect(eyeButton, &QPushButton::clicked, this, &AdjustmentCellManagerWidget::onEyeButtonToggled);
}

void AdjustmentCellManagerWidget::populatePresetList() {
    isRefreshingPresetList = true;
    QSignalBlocker blocker(presetList);
    presetList->clear();

    const auto& presets = (acm->getPresetListMode() == PresetScope::Local)
                              ? acm->getLocalPresets()
                              : acm->getGlobalPresets();

    for (const auto& preset : presets) {
        auto* item = new QListWidgetItem(preset->name, presetList);
        item->setData(Qt::UserRole, preset->dataId.toString());
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    isRefreshingPresetList = false;
}

void AdjustmentCellManagerWidget::updatePresetList() {
    populatePresetList();
}

void AdjustmentCellManagerWidget::updateActiveCell() {
    auto activeCell = acm->getActiveCell();
    if (activeCell) {
        activeCellNameLabel->setText("Active: " + activeCell->name);
        eyeButton->setChecked(activeCell->enabled);
    } else {
        activeCellNameLabel->setText("No cell selected");
        eyeButton->setChecked(false);
    }
}

void AdjustmentCellManagerWidget::onPresetScopeChanged(int modeId) {
    PresetScope newMode = (modeId == 1) ? PresetScope::Global : PresetScope::Local;
    acm->setPresetListMode(newMode);
    updatePresetList();
}

void AdjustmentCellManagerWidget::onPresetSelected(QListWidgetItem* item) {
    if (!item) {
        return;
    }

    auto preset = findPresetById(acm, item->data(Qt::UserRole).toString());
    if (!preset) {
        return;
    }

    auto cellData = acm->getCellData(preset->dataId);
    if (cellData) {
        acm->setActiveCell(cellData);
        updateActiveCell();
        emit presetActivated();
    }
}

void AdjustmentCellManagerWidget::clearPresetSelection() {
    QSignalBlocker blocker(presetList);
    presetList->clearSelection();
    presetList->setCurrentItem(nullptr);
}

void AdjustmentCellManagerWidget::onPresetItemChanged(QListWidgetItem* item) {
    if (isRefreshingPresetList || !item) {
        return;
    }

    auto preset = findPresetById(acm, item->data(Qt::UserRole).toString());
    if (!preset) {
        return;
    }

    const QString newName = item->text().trimmed();
    if (newName.isEmpty()) {
        QSignalBlocker blocker(presetList);
        item->setText(preset->name);
        return;
    }

    acm->renamePreset(preset, newName);

    // Keep active cell label in sync if renamed preset is currently active.
    updateActiveCell();
}

void AdjustmentCellManagerWidget::onRemovePresetClicked() {
    auto item = presetList->currentItem();
    if (!item) return;

    auto preset = findPresetById(acm, item->data(Qt::UserRole).toString());
    if (preset) {
        acm->removePreset(preset);
        updatePresetList();
    }
}

void AdjustmentCellManagerWidget::onApplyClicked() {
    acm->apply();
    emit appliedToImages();
    qDebug() << "Cell applied to selected images";
}

void AdjustmentCellManagerWidget::onSaveAsPresetClicked() {
    bool isGlobal = acm->getPresetListMode() == PresetScope::Global;
    acm->createPreset("New Preset", isGlobal);
    updatePresetList();
}

void AdjustmentCellManagerWidget::onApplyModeChanged(int modeId) {
    auto newMode = (modeId == 1)
                       ? AdjustmentCellManager::ApplyMode::Link
                       : AdjustmentCellManager::ApplyMode::Copy;
    acm->setApplyMode(newMode);
}

void AdjustmentCellManagerWidget::onEyeButtonToggled() {
    auto activeCell = acm->getActiveCell();
    if (activeCell) {
        acm->changeCellVisibility(activeCell, eyeButton->isChecked());
    }
}
