#include "adjustmentcellwidget.h"
#include "uiconstants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>
#include <QFrame>
#include <QMouseEvent>
#include <QApplication>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QScrollArea>
#include <QScrollBar>
#include <QWheelEvent>
#include <QCursor>

#include "../QtAwesome/QtAwesome.h"

#include <algorithm>

namespace {
fa::QtAwesome* cellAwesome()
{
    static fa::QtAwesome* awesome = []() {
        auto* a = new fa::QtAwesome();
        a->initFontAwesome();
        return a;
    }();
    return awesome;
}
}

AdjustmentCellWidget::AdjustmentCellWidget(AdjustmentCell* cell, QWidget* parent)
    : QWidget(parent), cell(cell)
{
    if (!cell) {
        return;
    }

    installEventFilter(this);
    setObjectName("adjustmentCellWidgetRoot");
    setAttribute(Qt::WA_StyledBackground, true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->setSpacing(8);

    // Header option buttons
    auto* headerWidget = new QWidget(this);
    headerWidget->installEventFilter(this);
    auto* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    const QString displayName = (cell->data && !cell->data->name.isEmpty())
        ? cell->data->name
        : QStringLiteral("Cell");
    nameEdit = new QLineEdit(displayName, headerWidget);
    nameEdit->installEventFilter(this);
    nameEdit->setReadOnly(true);
    nameEdit->setFrame(false);
    nameEdit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameEdit->setStyleSheet("QLineEdit { border: none; background: transparent; padding: 0; margin: 0; }");
    QFont boldFont = nameEdit->font();
    boldFont.setBold(true);
    nameEdit->setFont(boldFont);

    auto* hideBtn = new QToolButton(headerWidget);
    auto* eyeBtn = new QToolButton(headerWidget);
    auto* unlinkBtn = new QToolButton(headerWidget);
    auto* removeBtn = new QToolButton(headerWidget);
    hideBtn->installEventFilter(this);
    eyeBtn->installEventFilter(this);
    unlinkBtn->installEventFilter(this);
    removeBtn->installEventFilter(this);
    hideBtn->setFixedSize(24, 20);
    eyeBtn->setFixedSize(24, 20);
    unlinkBtn->setFixedSize(24, 20);
    removeBtn->setFixedSize(24, 20);
    hideBtn->setAutoRaise(true);
    eyeBtn->setAutoRaise(true);
    unlinkBtn->setAutoRaise(true);
    removeBtn->setAutoRaise(true);

    hideBtn->setToolTip("Collapse/expand controls");
    eyeBtn->setToolTip("Toggle cell visibility");
    unlinkBtn->setToolTip("Unlink cell from shared source");
    removeBtn->setToolTip("Remove cell");
    hideBtn->setCursor(Qt::PointingHandCursor);
    eyeBtn->setCursor(Qt::PointingHandCursor);
    unlinkBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setCursor(Qt::PointingHandCursor);

    auto* awesome = cellAwesome();
        hideBtn->setIcon(awesome->icon(fa::fa_solid, cell->collapsed ? fa::fa_chevron_down : fa::fa_chevron_up));
    eyeBtn->setIcon(awesome->icon(fa::fa_solid, cell->visible ? fa::fa_eye : fa::fa_eye_slash));
    unlinkBtn->setIcon(awesome->icon(fa::fa_solid, fa::fa_unlink));
    removeBtn->setIcon(awesome->icon(fa::fa_solid, fa::fa_trash));

    unlinkBtn->setEnabled(cell->isLinked());

    headerLayout->addWidget(nameEdit);
    headerLayout->addStretch();
    headerLayout->addWidget(hideBtn);
    headerLayout->addWidget(eyeBtn);
    headerLayout->addWidget(unlinkBtn);
    headerLayout->addWidget(removeBtn);
    mainLayout->addWidget(headerWidget);

    // Separator
    auto* line = new QFrame(this);
    line->installEventFilter(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setFixedHeight(1);
    line->setStyleSheet("QFrame { border: none; background-color: rgba(255, 255, 255, 0.28); }");
    mainLayout->addWidget(line);

    // Container that holds all sliders (toggled by the hide button)
    auto* slidersContainer = new QWidget(this);
    slidersContainer->installEventFilter(this);
        isCollapsed = cell->collapsed;
    slidersContainer->setVisible(!isCollapsed);
    auto* slidersLayout = new QVBoxLayout(slidersContainer);
    slidersLayout->setContentsMargins(0, 0, 0, 0);
    slidersLayout->setSpacing(2);
    mainLayout->addWidget(slidersContainer);

    connect(hideBtn, &QToolButton::clicked, this, [this, slidersContainer, cell, hideBtn]()
    {
        emit cellActivated(cell);
        isCollapsed = !isCollapsed;
            cell->collapsed = isCollapsed;
        slidersContainer->setVisible(!isCollapsed);
        hideBtn->setIcon(cellAwesome()->icon(fa::fa_solid, isCollapsed ? fa::fa_chevron_down : fa::fa_chevron_up));
    });

    connect(eyeBtn, &QToolButton::clicked, this, [this, cell, eyeBtn]()
    {
        emit cellActivated(cell);
        cell->visible = !cell->visible;
        eyeBtn->setIcon(cellAwesome()->icon(fa::fa_solid, cell->visible ? fa::fa_eye : fa::fa_eye_slash));
        emit adjustmentChanged();
    });

    connect(unlinkBtn, &QToolButton::clicked, this, [this, cell]()
    {
        emit cellActivated(cell);
        emit unlinkCellRequested(cell);
    });

    connect(removeBtn, &QToolButton::clicked, this, [this, cell]()
    {
        emit removeCellRequested(cell);
    });

    connect(nameEdit, &QLineEdit::editingFinished, this, [this, cell]() {
        if (!nameEdit || nameEdit->isReadOnly()) {
            return;
        }

        const QString currentName = (cell->data && !cell->data->name.isEmpty())
            ? cell->data->name
            : QStringLiteral("Cell");
        const QString newName = nameEdit->text().trimmed();

        // Lock editing first. The rename signal can trigger a panel rebuild that destroys this widget.
        nameEdit->setReadOnly(true);

        if (!newName.isEmpty() && newName != currentName) {
            emit cellRenameRequested(cell, newName);
        } else {
            nameEdit->setText(currentName);
        }
    });

    // Create one slider row per adjustable attribute
    if (!cell->data) {
        return;
    }

    auto& adjustments = cell->data->adjustments;
    bool toneLabelAdded = false;
    bool colorLabelAdded = false;
    bool detailLabelAdded = false;

    auto addSectionLabel = [slidersContainer, slidersLayout](const QString& text) {
        auto* sectionLabel = new QLabel(text, slidersContainer);
        sectionLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        sectionLabel->setStyleSheet(QString("color: %1; font-weight: 600;").arg(appMutedTextColor));
        sectionLabel->setContentsMargins(0, 2, 0, 1);
        slidersLayout->addWidget(sectionLabel);
    };

    for (AdjType type : cell->displayOrder) {
        if (type == AdjType::Exposure && !toneLabelAdded) {
            addSectionLabel("Tone");
            toneLabelAdded = true;
        }
        if (type == AdjType::Saturation && !colorLabelAdded) {
            addSectionLabel("Color");
            colorLabelAdded = true;
        }
        if (type == AdjType::Denoise && !detailLabelAdded) {
            addSectionLabel("Detail");
            detailLabelAdded = true;
        }

        auto it = adjustments.find(type);
        if (it == adjustments.end()) continue;
        auto& adjustment = it->second;
        for (auto& attr : adjustment->attributes) {
            if (!attr.isAdjustable)
                continue;

            // Name + value row
            auto* rowWidget = new QWidget(slidersContainer);
            rowWidget->installEventFilter(this);
            auto* rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(4);

            auto* attrNameLabel = new QLabel(QString::fromStdString(attr.name), rowWidget);
            attrNameLabel->installEventFilter(this);
            attrNameLabel->setMinimumWidth(70);

            auto* valueLabel = new QLabel(QString::number(attr.value, 'f', 2), rowWidget);
            valueLabel->installEventFilter(this);
            valueLabel->setMinimumWidth(36);
            valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

            rowLayout->addWidget(attrNameLabel);
            rowLayout->addStretch();
            rowLayout->addWidget(valueLabel);
            slidersLayout->addWidget(rowWidget);

            // Slider: integer range [0, 1000] mapped linearly to [attr.min, attr.max]
            auto* slider = new QSlider(Qt::Horizontal, slidersContainer);
            slider->installEventFilter(this);
            slider->setRange(0, 1000);
            slider->setFixedHeight(18);
            slider->setStyleSheet(
                QStringLiteral(
                    "QSlider {"
                    " background: transparent;"
                    " }"
                    "QSlider::groove:horizontal {"
                    " height: 3px;"
                    " background: %1;"
                    " border-radius: 2px;"
                    " }"
                    "QSlider::sub-page:horizontal {"
                    " background: %2;"
                    " border-radius: 2px;"
                    " }"
                    "QSlider::add-page:horizontal {"
                    " background: %1;"
                    " border-radius: 2px;"
                    " }"
                    "QSlider::handle:horizontal {"
                    " background-color: #ffffff;"
                    " border: 1px solid rgba(0, 0, 0, 0.18);"
                    " width: 10px;"
                    " height: 10px;"
                    " margin: -4px 0;"
                    " border-radius: 5px;"
                    " }"
                ).arg(appSurfacePressedColor, appMutedTextColor)
            );

            float range = attr.max - attr.min;
            int initVal = static_cast<int>((attr.value - attr.min) / range * 1000.f);
            slider->setValue(std::clamp(initVal, 0, 1000));

            // Connect slider change to attribute update and emit adjustmentChanged()
            AdjAttribute* attrPtr = &attr;
            connect(slider, &QSlider::valueChanged, this, [this, cell, attrPtr, lbl = valueLabel](int v)
            {
                emit cellActivated(cell);
                attrPtr->value = attrPtr->min + (v / 1000.f) * (attrPtr->max - attrPtr->min);
                lbl->setText(QString::number(attrPtr->value, 'f', 2));
                emit adjustmentChanged();
            });

            connect(slider, &QSlider::sliderReleased, this, [this, cell]()
            {
                emit sliderReleased(cell);
                emit cellActivated(cell);
            });

            slidersLayout->addWidget(slider);
        }
    }

    applyVisualState();
}

void AdjustmentCellWidget::setActive(bool active)
{
    if (isActive == active) {
        return;
    }

    isActive = active;
    applyVisualState();
}

void AdjustmentCellWidget::updateVisualState()
{
    if (nameEdit && nameEdit->isReadOnly() && cell) {
        const QString displayName = (cell->data && !cell->data->name.isEmpty())
            ? cell->data->name
            : QStringLiteral("Cell");
        if (nameEdit->text() != displayName) {
            nameEdit->setText(displayName);
        }
    }

    applyVisualState();
}

void AdjustmentCellWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && cell) {
        emit cellActivated(cell);
    }

    QWidget::mousePressEvent(event);
}

bool AdjustmentCellWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (!event || !cell) {
        return QWidget::eventFilter(watched, event);
    }

    // Prevent mouse wheel from changing slider values, but keep scroll area scrolling.
    if (event->type() == QEvent::Wheel && qobject_cast<QAbstractSlider*>(watched)) {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);

        QWidget* parent = this;
        while (parent) {
            if (auto* scrollArea = qobject_cast<QScrollArea*>(parent)) {
                auto* scrollBar = scrollArea->verticalScrollBar();
                const int steps = wheelEvent->angleDelta().y() / 120;
                if (steps != 0) {
                    scrollBar->setValue(scrollBar->value() - (steps * scrollBar->singleStep() * 3));
                }
                break;
            }
            parent = parent->parentWidget();
        }

        return true;
    }

    if (watched == nameEdit && event->type() == QEvent::MouseButtonDblClick) {
        nameEdit->setReadOnly(false);
        nameEdit->setFocus();
        nameEdit->selectAll();
        trackingForDrag = false;
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            // Only initiate drag tracking from non-interactive elements
            if (!qobject_cast<QAbstractButton*>(watched)
                && !qobject_cast<QAbstractSlider*>(watched)
                && !(watched == nameEdit && !nameEdit->isReadOnly())) {
                dragPressPos = QCursor::pos();
                trackingForDrag = true;
            }
        }
        emit cellActivated(cell);
    } else if (event->type() == QEvent::MouseMove && trackingForDrag) {
        auto* me = static_cast<QMouseEvent*>(event);
        if ((QCursor::pos() - dragPressPos).manhattanLength() >= QApplication::startDragDistance()) {
            trackingForDrag = false;
            emit dragInitiated(this, QCursor::pos());
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        trackingForDrag = false;
    }

    return QWidget::eventFilter(watched, event);
}

void AdjustmentCellWidget::applyVisualState()
{
    bool isEnabled = cell && cell->data && cell->data->enabled;
    const QString rootTransparentChildren =
        QString("QWidget#adjustmentCellWidgetRoot QWidget { background: transparent; }")
        + QString("QWidget#adjustmentCellWidgetRoot QLabel { background: transparent; }")
        + QString("QWidget#adjustmentCellWidgetRoot QSlider { background: transparent; }");
    const QString rootBaseStyle =
        QString("QWidget#adjustmentCellWidgetRoot {")
        + QString(" background-color: %1;")
        + QString(" border-radius: 10px;")
        + QString(" border: %2;")
        + QString(" }");

    if (!isEnabled) {
        // Grayed out disabled state
        const QString disabledBorderStyle = isActive
            ? QString("2px solid %1").arg(appActiveHighlightColor)
            : QStringLiteral("none");
        setStyleSheet(
            rootBaseStyle.arg(appPanelBackgroundColor, disabledBorderStyle)
            + QString("QWidget#adjustmentCellWidgetRoot { color: rgba(255, 255, 255, 0.55); }")
            + rootTransparentChildren
        );
        setEnabled(false);
    } else {
        // Normal enabled state
        if (isActive) {
            setStyleSheet(rootBaseStyle.arg(appPanelBackgroundColor, QString("2px solid %1").arg(appActiveHighlightColor)) + rootTransparentChildren);
        } else {
            setStyleSheet(rootBaseStyle.arg(appPanelBackgroundColor, "none") + rootTransparentChildren);
        }
        setEnabled(true);
    }
}
