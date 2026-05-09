#include "adjustmentcellwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QSlider>
#include <QFrame>
#include <QMouseEvent>

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

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 8);
    mainLayout->setSpacing(4);

    // Header option buttons
    auto* headerWidget = new QWidget(this);
    headerWidget->installEventFilter(this);
    auto* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    const QString displayName = (cell->data && !cell->data->name.isEmpty())
        ? cell->data->name
        : cell->instanceName;
    auto* nameLabel = new QLabel(displayName, headerWidget);
    nameLabel->installEventFilter(this);
    QFont boldFont = nameLabel->font();
    boldFont.setBold(true);
    nameLabel->setFont(boldFont);

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

    auto* awesome = cellAwesome();
    hideBtn->setIcon(awesome->icon(fa::fa_solid, fa::fa_chevron_up));
    eyeBtn->setIcon(awesome->icon(fa::fa_solid, cell->visible ? fa::fa_eye : fa::fa_eye_slash));
    unlinkBtn->setIcon(awesome->icon(fa::fa_solid, fa::fa_unlink));
    removeBtn->setIcon(awesome->icon(fa::fa_solid, fa::fa_trash));

    unlinkBtn->setEnabled(cell->isLinked());

    headerLayout->addWidget(nameLabel);
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
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // Container that holds all sliders (toggled by the hide button)
    auto* slidersContainer = new QWidget(this);
    slidersContainer->installEventFilter(this);
    slidersContainer->setVisible(!isCollapsed);
    auto* slidersLayout = new QVBoxLayout(slidersContainer);
    slidersLayout->setContentsMargins(0, 0, 0, 0);
    slidersLayout->setSpacing(4);
    mainLayout->addWidget(slidersContainer);

    connect(hideBtn, &QToolButton::clicked, this, [this, slidersContainer, cell, hideBtn]()
    {
        emit cellActivated(cell);
        isCollapsed = !isCollapsed;
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

    // Create one slider row per adjustable attribute
    if (!cell->data) {
        return;
    }

    auto& adjustments = cell->data->adjustments;
    for (AdjType type : cell->displayOrder) {
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

            auto* attrNameLabel = new QLabel(QString::fromStdString(attr.name), rowWidget);
            attrNameLabel->installEventFilter(this);
            attrNameLabel->setMinimumWidth(70);

            auto* valueLabel = new QLabel(QString::number(attr.value, 'f', 2), rowWidget);
            valueLabel->installEventFilter(this);
            valueLabel->setMinimumWidth(40);
            valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

            rowLayout->addWidget(attrNameLabel);
            rowLayout->addStretch();
            rowLayout->addWidget(valueLabel);
            slidersLayout->addWidget(rowWidget);

            // Slider: integer range [0, 1000] mapped linearly to [attr.min, attr.max]
            auto* slider = new QSlider(Qt::Horizontal, slidersContainer);
            slider->installEventFilter(this);
            slider->setRange(0, 1000);

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

void AdjustmentCellWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && cell) {
        emit cellActivated(cell);
    }

    QWidget::mousePressEvent(event);
}

bool AdjustmentCellWidget::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);

    if (event && event->type() == QEvent::MouseButtonPress && cell) {
        emit cellActivated(cell);
    }

    return QWidget::eventFilter(watched, event);
}

void AdjustmentCellWidget::applyVisualState()
{
    if (isActive) {
        setStyleSheet("QWidget#adjustmentCellWidgetRoot { background-color: rgba(255, 255, 255, 0.14); border-radius: 4px; }");
    } else {
        setStyleSheet("QWidget#adjustmentCellWidgetRoot { background-color: rgba(255, 255, 255, 0.05); border-radius: 4px; }");
    }
}
