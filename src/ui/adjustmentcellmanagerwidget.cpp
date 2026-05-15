#include "adjustmentcellmanagerwidget.h"
#include "uiconstants.h"
#include "../core/image.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QLabel>
#include <QDebug>
#include <QSignalBlocker>
#include <QButtonGroup>
#include <QEvent>
#include <QMouseEvent>
#include <QFrame>

namespace {
std::shared_ptr<Preset> findPresetById(std::shared_ptr<AdjustmentManager> acm, const QString& idStr) {
    const QUuid id = QUuid(idStr);
    if (id.isNull()) {
        return nullptr;
    }

    const auto& localPresets = acm->getLocalPresets();
    for (const auto& preset : localPresets) {
        if (preset && preset->data && preset->data->id == id) {
            return preset;
        }
    }

    const auto& globalPresets = acm->getGlobalPresets();
    for (const auto& preset : globalPresets) {
        if (preset && preset->data && preset->data->id == id) {
            return preset;
        }
    }

    return nullptr;
}

void setApplyModeDependentState(
    AdjustmentManager::ApplyMode mode,
    QLabel* autoPresetLabel,
    QPushButton* autoPresetOffButton,
    QPushButton* autoPresetOnButton
) {
    const bool enabled = (mode == AdjustmentManager::ApplyMode::Link);
    autoPresetLabel->setEnabled(enabled);
    autoPresetOffButton->setEnabled(enabled);
    autoPresetOnButton->setEnabled(enabled);
}
}

AdjustmentCellManagerWidget::AdjustmentCellManagerWidget(std::shared_ptr<AdjustmentManager> acm, QWidget* parent)
    : QWidget(parent), acm(acm) {
    setupUI();
    setupConnections();

    connect(acm.get(), &AdjustmentManager::linkedCellDataChanged, this, [this](QUuid cellDataId) {
        if (cellDataId.isNull()) {
            return;
        }

        // Update preset list item names if the linked cell data's name changed.
        const QString idStr = cellDataId.toString();
        for (int i = 0; i < presetList->count(); ++i) {
            auto* item = presetList->item(i);
            if (!item || item->data(Qt::UserRole).toString() != idStr) {
                continue;
            }

            auto preset = findPresetById(this->acm, idStr);
            if (preset && item->text() != preset->name) {
                QSignalBlocker blocker(presetList);
                item->setText(preset->name);
            }
        }

        updateActiveCell();
    });

    updatePresetList();
}

void AdjustmentCellManagerWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto styleSectionHeader = [](QLabel* label) {
        QFont headerFont = label->font();
        headerFont.setBold(true);
        headerFont.setPointSize(headerFont.pointSize() + 2);
        label->setFont(headerFont);
    };

    // Preset list
    mainLayout->addSpacing(20);
    
    auto* presetsHeaderWidget = new QWidget(this);
    auto* presetsHeaderLayout = new QHBoxLayout(presetsHeaderWidget);
    presetsHeaderLayout->setContentsMargins(8, 0, 0, 0);
    
    presetsLabel = new QLabel("Presets", presetsHeaderWidget);
    styleSectionHeader(presetsLabel);
    presetsHeaderLayout->addWidget(presetsLabel);
    presetsHeaderLayout->addStretch();
    
    mainLayout->addWidget(presetsHeaderWidget);

    auto* presetsDivider = new QFrame(this);
    presetsDivider->setFrameShape(QFrame::HLine);
    presetsDivider->setFrameShadow(QFrame::Plain);
    presetsDivider->setFixedHeight(1);
    presetsDivider->setStyleSheet("QFrame { border: none; background-color: rgba(255, 255, 255, 0.20); }");
    mainLayout->addWidget(presetsDivider);

    mainLayout->addSpacing(10);

    presetList = new QListWidget(this);
    presetList->setMaximumHeight(150);
    presetList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    presetList->setStyleSheet("QListWidget::item { padding: 4px 8px; }");
    mainLayout->addWidget(presetList);

    // Preset scope switch (Local/Global)
    presetScopeSwitchWidget = new QWidget(this);
    auto* presetScopeLayout = new QHBoxLayout(presetScopeSwitchWidget);
    presetScopeLayout->setContentsMargins(4, 4, 4, 4);
    presetScopeLayout->setSpacing(0);

    localScopeButton = new QPushButton("Local", presetScopeSwitchWidget);
    globalScopeButton = new QPushButton("Global", presetScopeSwitchWidget);
    localScopeButton->setCheckable(true);
    globalScopeButton->setCheckable(true);
    localScopeButton->setToolTip("Show local presets");
    globalScopeButton->setToolTip("Show global presets");
    localScopeButton->setCursor(Qt::PointingHandCursor);
    globalScopeButton->setCursor(Qt::PointingHandCursor);

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

    // Remove preset button
    removePresetButton = new QPushButton("Remove Selected Preset", this);
    removePresetButton->setStyleSheet(baseButtonStyle);
    removePresetButton->setEnabled(false);  // Disabled until a preset is selected
    removePresetButton->setToolTip("Remove the selected preset from the list (does not delete the adjustment cells themselves)");
    removePresetButton->setCursor(Qt::PointingHandCursor);
    mainLayout->addWidget(removePresetButton);

    mainLayout->addSpacing(20);

    // Active cell controls container (shown/hidden based on active cell)
    activeCellControlsWidget = new QWidget(this);
    activeCellControlsWidget->setVisible(false);  // hidden by default until a cell is selected
    auto* activeCellControlsLayout = new QVBoxLayout(activeCellControlsWidget);
    activeCellControlsLayout->setContentsMargins(0, 0, 0, 0);
    activeCellControlsLayout->addSpacing(8);
    mainLayout->addWidget(activeCellControlsWidget);

    // Active cell section header + dynamic name on the same row
    auto* activeCellHeaderRow = new QWidget(activeCellControlsWidget);
    auto* activeCellHeaderRowLayout = new QHBoxLayout(activeCellHeaderRow);
    activeCellHeaderRowLayout->setContentsMargins(8, 0, 8, 0);
    //activeCellHeaderRowLayout->addSpacing(8);

    auto* activeCellSectionLabel = new QLabel("Active cell/preset", activeCellHeaderRow);
    styleSectionHeader(activeCellSectionLabel);
    activeCellHeaderRowLayout->addWidget(activeCellSectionLabel);
    activeCellHeaderRowLayout->addStretch();

    // Dynamic label updated by updateActiveCell(). Uses same size as header but lighter weight.
    activeCellNameLabel = new QLabel("No cell selected", activeCellHeaderRow);
    activeCellNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont activeCellNameFont = activeCellSectionLabel->font();
    activeCellNameFont.setBold(false);
    activeCellNameFont.setWeight(QFont::Light);
    activeCellNameLabel->setFont(activeCellNameFont);
    //activeCellNameLabel->setStyleSheet(QString("color: %1;").arg(appMutedTextColor));
    activeCellHeaderRowLayout->addWidget(activeCellNameLabel);

    activeCellControlsLayout->addWidget(activeCellHeaderRow);

    auto* activeCellDivider = new QFrame(activeCellControlsWidget);
    activeCellDivider->setFrameShape(QFrame::HLine);
    activeCellDivider->setFrameShadow(QFrame::Plain);
    activeCellDivider->setFixedHeight(1);
    activeCellDivider->setStyleSheet("QFrame { border: none; background-color: rgba(255, 255, 255, 0.20); }");
    activeCellControlsLayout->addWidget(activeCellDivider);

    activeCellControlsLayout->addSpacing(10);

    // Enabled state switch (Enabled/Disabled)
    enabledStateWidget = new QWidget(activeCellControlsWidget);
    auto* enabledStateLayout = new QHBoxLayout(enabledStateWidget);
    enabledStateLayout->setContentsMargins(4, 4, 4, 4);
    //enabledStateLayout->setSpacing(0);

    enabledButton = new QPushButton("Enabled", enabledStateWidget);
    disabledButton = new QPushButton("Disabled", enabledStateWidget);
    enabledButton->setCheckable(true);
    disabledButton->setCheckable(true);
    enabledButton->setToolTip("Enable active cell processing");
    disabledButton->setToolTip("Disable active cell processing");
    enabledButton->setCursor(Qt::PointingHandCursor);
    disabledButton->setCursor(Qt::PointingHandCursor);

    enabledStateGroup = new QButtonGroup(enabledStateWidget);
    enabledStateGroup->setExclusive(true);
    enabledStateGroup->addButton(enabledButton, 1);
    enabledStateGroup->addButton(disabledButton, 0);

    enabledStateLayout->addWidget(enabledButton);
    enabledStateLayout->addWidget(disabledButton);
    enabledStateWidget->setStyleSheet(switchStyle);
    activeCellControlsLayout->addWidget(enabledStateWidget);

    // Save preset button is now a full-width action below the enabled/disabled control.
    saveAsPresetButton = new QPushButton("Save as Preset", activeCellControlsWidget);
    saveAsPresetButton->setStyleSheet(baseButtonStyle);
    saveAsPresetButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    saveAsPresetButton->setToolTip("Save active cell settings as a preset");
    saveAsPresetButton->setCursor(Qt::PointingHandCursor);
    activeCellControlsLayout->addWidget(saveAsPresetButton);

    activeCellControlsLayout->addSpacing(12);

    // Apply controls panel
    QWidget* applyControlsWidget = new QWidget(activeCellControlsWidget);
    applyControlsWidget->setObjectName("applyControlsWidget");
    applyControlsWidget->setStyleSheet(applyControlsPanelStyle);
    applyControlsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* applyControlsLayout = new QVBoxLayout(applyControlsWidget);
    applyControlsLayout->setContentsMargins(10, 10, 10, 10);

    QLabel* applyControlsTitle = new QLabel("Cell/Preset Application", applyControlsWidget);
    applyControlsTitle->setObjectName("applyControlsTitle");
    applyControlsTitle->setAlignment(Qt::AlignCenter);
    applyControlsTitle->setAutoFillBackground(true);
    applyControlsTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    applyControlsLayout->addWidget(applyControlsTitle);

    applyControlsLayout->addSpacing(5);

    // Apply button with mode
    applyButton = new QPushButton("Apply", applyControlsWidget);
    applyButton->setStyleSheet(baseButtonStyle);
    applyButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    applyButton->setToolTip("Apply active cell to selected images");
    applyButton->setCursor(Qt::PointingHandCursor);
    applyControlsLayout->addWidget(applyButton);

    applyModeSwitchWidget = new QWidget(applyControlsWidget);
    auto* modeSwitchLayout = new QHBoxLayout(applyModeSwitchWidget);
    modeSwitchLayout->setContentsMargins(2, 2, 2, 2);
    modeSwitchLayout->setSpacing(0);

    copyModeButton = new QPushButton("Copy", applyModeSwitchWidget);
    linkModeButton = new QPushButton("Link", applyModeSwitchWidget);
    copyModeButton->setCheckable(true);
    linkModeButton->setCheckable(true);
    copyModeButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    linkModeButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    copyModeButton->setToolTip("Apply as independent copy of the cell (deep copy)");
    linkModeButton->setToolTip("Apply as instance dynamically linked to the original cell (changes to original will affect linked copies and vice versa)");
    copyModeButton->setCursor(Qt::PointingHandCursor);
    linkModeButton->setCursor(Qt::PointingHandCursor);

    applyModeGroup = new QButtonGroup(applyModeSwitchWidget);
    applyModeGroup->setExclusive(true);
    applyModeGroup->addButton(copyModeButton, 0);
    applyModeGroup->addButton(linkModeButton, 1);

    if (acm->getApplyMode() == AdjustmentManager::ApplyMode::Link) {
        linkModeButton->setChecked(true);
    } else {
        copyModeButton->setChecked(true);
    }

    modeSwitchLayout->addWidget(copyModeButton);
    modeSwitchLayout->addWidget(linkModeButton);
    applyModeSwitchWidget->setStyleSheet(switchStyle);
    applyControlsLayout->addWidget(applyModeSwitchWidget);

    applyControlsLayout->addSpacing(10);

    // Option: add linked copy as preset after apply (link mode only)
    QWidget* autoPresetRowWidget = new QWidget(applyControlsWidget);
    autoPresetRowWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    autoPresetRowWidget->setStyleSheet("background: transparent;");

    QHBoxLayout* autoPresetLayout = new QHBoxLayout(autoPresetRowWidget);
    autoPresetLayout->setContentsMargins(0, 0, 0, 0);
    autoPresetLayout->setSpacing(8);

    autoPresetLabel = new QLabel("Add linked copy as preset", applyControlsWidget);
    autoPresetLabel->setAlignment(Qt::AlignLeft);
    autoPresetLayout->addWidget(autoPresetLabel);
    autoPresetLayout->addStretch();

    autoPresetSwitchWidget = new QWidget(activeCellControlsWidget);
    auto* autoPresetSwitchLayout = new QHBoxLayout(autoPresetSwitchWidget);
    autoPresetSwitchLayout->setContentsMargins(2, 2, 2, 2);
    autoPresetSwitchLayout->setSpacing(0);

    autoPresetOffButton = new QPushButton("", autoPresetSwitchWidget);
    autoPresetOnButton = new QPushButton("", autoPresetSwitchWidget);
    autoPresetSwitchWidget->setObjectName("autoPresetSwitchWidget");
    autoPresetOffButton->setObjectName("autoPresetOffButton");
    autoPresetOnButton->setObjectName("autoPresetOnButton");

    autoPresetOffButton->setCheckable(true);
    autoPresetOnButton->setCheckable(true);
    autoPresetOffButton->setFixedSize(18, 14);
    autoPresetOnButton->setFixedSize(18, 14);
    autoPresetOffButton->setToolTip("Do not add the cell as a linked preset after apply");
    //autoPresetOnButton->setToolTip("Add the cell as a linked preset after apply");
    //autoPresetSwitchWidget->setToolTip("Add the cell as a linked preset after apply");
    autoPresetSwitchWidget->setCursor(Qt::PointingHandCursor);

    // Make the entire compact switch area clickable.
    autoPresetOffButton->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    autoPresetOnButton->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    autoPresetSwitchGroup = new QButtonGroup(autoPresetSwitchWidget);
    autoPresetSwitchGroup->setExclusive(true);
    autoPresetSwitchGroup->addButton(autoPresetOffButton, 0);
    autoPresetSwitchGroup->addButton(autoPresetOnButton, 1);
    autoPresetOnButton->setChecked(true);

    autoPresetSwitchLayout->addWidget(autoPresetOffButton);
    autoPresetSwitchLayout->addWidget(autoPresetOnButton);
    autoPresetSwitchWidget->setStyleSheet(compactSwitchStyle);
    autoPresetLayout->addWidget(autoPresetSwitchWidget);
    autoPresetSwitchWidget->installEventFilter(this);
    applyModeSwitchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    autoPresetLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    autoPresetSwitchWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    applyControlsLayout->addWidget(autoPresetRowWidget);
    activeCellControlsLayout->addWidget(applyControlsWidget);
    activeCellControlsLayout->addStretch();

    mainLayout->addStretch();

    setLayout(mainLayout);
}

void AdjustmentCellManagerWidget::setupConnections() {
    connect(presetScopeGroup, &QButtonGroup::idClicked, this, &AdjustmentCellManagerWidget::onPresetScopeChanged);
    connect(presetList, &QListWidget::itemSelectionChanged, this, [this]() {
        if (auto item = presetList->currentItem()) {
            onPresetSelected(item);
        }
        removePresetButton->setEnabled(presetList->currentItem() != nullptr);
    });
    connect(presetList, &QListWidget::itemClicked, this, &AdjustmentCellManagerWidget::onPresetSelected);
    connect(removePresetButton, &QPushButton::clicked, this, &AdjustmentCellManagerWidget::onRemovePresetClicked);
    connect(presetList, &QListWidget::itemChanged, this, &AdjustmentCellManagerWidget::onPresetItemChanged);
    connect(applyButton, &QPushButton::clicked, this, &AdjustmentCellManagerWidget::onApplyClicked);
    connect(saveAsPresetButton, &QPushButton::clicked, this, &AdjustmentCellManagerWidget::onSaveAsPresetClicked);
    connect(applyModeGroup, &QButtonGroup::idClicked, this, &AdjustmentCellManagerWidget::onApplyModeChanged);
    connect(enabledStateGroup, &QButtonGroup::idClicked, this, &AdjustmentCellManagerWidget::onEnabledStateChanged);
    connect(autoPresetSwitchGroup, &QButtonGroup::idClicked, this, &AdjustmentCellManagerWidget::onAutoPresetSwitchChanged);

    setApplyModeDependentState(acm->getApplyMode(), autoPresetLabel, autoPresetOffButton, autoPresetOnButton);
}

void AdjustmentCellManagerWidget::populatePresetList() {
    isRefreshingPresetList = true;
    QSignalBlocker blocker(presetList);
    presetList->clear();

    const auto& presets = (acm->getPresetListMode() == PresetScope::Local)
                              ? acm->getLocalPresets()
                              : acm->getGlobalPresets();

    for (const auto& preset : presets) {
        if (!preset || !preset->data) {
            continue;
        }
        auto* item = new QListWidgetItem(preset->name, presetList);
        QFont itemFont = item->font();
        itemFont.setPointSize(itemFont.pointSize() + 1);
        item->setFont(itemFont);
        item->setData(Qt::UserRole, preset->data->id.toString());
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
        activeCellNameLabel->setText(activeCell->name);
        activeCellControlsWidget->setVisible(true);
        
        // Update enabled state buttons
        QSignalBlocker blocker(enabledStateGroup);
        if (activeCell->isEnabled) {
            enabledButton->setChecked(true);
        } else {
            disabledButton->setChecked(true);
        }
        
        // Always enable the switch for all cells
        enabledButton->setEnabled(true);
        disabledButton->setEnabled(true);
    } else {
        activeCellNameLabel->setText("No cell selected");
        activeCellControlsWidget->setVisible(false);
        enabledButton->setEnabled(false);
        disabledButton->setEnabled(false);
    }
}

void AdjustmentCellManagerWidget::onPresetScopeChanged(int modeId) {
    // Save the currently selected preset ID before changing scope
    if (auto item = presetList->currentItem()) {
        previouslySelectedPresetId = item->data(Qt::UserRole).toString();
    } else {
        previouslySelectedPresetId.clear();
    }
    
    PresetScope newMode = (modeId == 1) ? PresetScope::Global : PresetScope::Local;
    acm->setPresetListMode(newMode);
    updatePresetList();
}

void AdjustmentCellManagerWidget::onPresetSelected(QListWidgetItem* item) {
    if (!item) {
        return;
    }

    // Save the selected preset ID
    previouslySelectedPresetId = item->data(Qt::UserRole).toString();
    
    auto preset = findPresetById(acm, item->data(Qt::UserRole).toString());
    if (!preset) {
        return;
    }

    auto cellData = preset->data;
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
    previouslySelectedPresetId.clear();  // Clear the saved preset ID
    removePresetButton->setEnabled(false);  // Disable button when preset is cleared
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

    // Optional automation: if applying in link mode, add current linked data as a preset if missing.
    const bool autoAddPreset = autoPresetOnButton && autoPresetOnButton->isChecked();
    if (autoAddPreset && acm->getApplyMode() == AdjustmentManager::ApplyMode::Link) {
        const bool isGlobal = (acm->getPresetListMode() == PresetScope::Global);
        acm->createPresetFromActiveCellIfMissing(isGlobal);
        updatePresetList();
    }

    emit appliedToImages();
    // qDebug() << "Cell applied to selected images";
}

void AdjustmentCellManagerWidget::onSaveAsPresetClicked() {
    bool isGlobal = acm->getPresetListMode() == PresetScope::Global;
    acm->createPreset("New Preset", isGlobal);
    updatePresetList();
}

void AdjustmentCellManagerWidget::onApplyModeChanged(int modeId) {
    auto newMode = (modeId == 1)
                       ? AdjustmentManager::ApplyMode::Link
                       : AdjustmentManager::ApplyMode::Copy;
    acm->setApplyMode(newMode);
    setApplyModeDependentState(newMode, autoPresetLabel, autoPresetOffButton, autoPresetOnButton);
}

void AdjustmentCellManagerWidget::onEnabledStateChanged(int stateId) {
    auto activeCell = acm->getActiveCell();
    if (activeCell) {
        bool enabled = (stateId == 1);  // 1 = Enabled, 0 = Disabled
        acm->changeCellVisibility(activeCell, enabled);
        // Notify listeners to re-render (works for both linked and non-linked cells)
        acm->notifyCellDataChanged(activeCell);
    }
}

void AdjustmentCellManagerWidget::onAutoPresetSwitchChanged(int stateId) {
    Q_UNUSED(stateId);
}

bool AdjustmentCellManagerWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == autoPresetSwitchWidget && event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && autoPresetOnButton->isEnabled() && autoPresetOffButton->isEnabled()) {
            const bool isOn = autoPresetOnButton->isChecked();
            QSignalBlocker blocker(autoPresetSwitchGroup);
            autoPresetOnButton->setChecked(!isOn);
            autoPresetOffButton->setChecked(isOn);
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

