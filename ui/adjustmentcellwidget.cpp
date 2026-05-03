#include "adjustmentcellwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QFrame>

#include <algorithm>

AdjustmentCellWidget::AdjustmentCellWidget(AdjustmentCell* cell, QWidget* parent)
    : QWidget(parent), cell(cell)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 8);
    mainLayout->setSpacing(4);

    // Header option buttons
    auto* headerWidget = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    auto* nameLabel = new QLabel(QString::fromStdString(cell->name), headerWidget);
    QFont boldFont = nameLabel->font();
    boldFont.setBold(true);
    nameLabel->setFont(boldFont);

    auto* hideBtn = new QPushButton("H", headerWidget);
    auto* removeBtn = new QPushButton("X", headerWidget);
    hideBtn->setFixedHeight(20);
    removeBtn->setFixedHeight(20);

    headerLayout->addWidget(nameLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(hideBtn);
    headerLayout->addWidget(removeBtn);
    mainLayout->addWidget(headerWidget);

    // Separator
    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    // Container that holds all sliders (toggled by the hide button)
    auto* slidersContainer = new QWidget(this);
    slidersContainer->setVisible(cell->isVisible);
    auto* slidersLayout = new QVBoxLayout(slidersContainer);
    slidersLayout->setContentsMargins(0, 0, 0, 0);
    slidersLayout->setSpacing(4);
    mainLayout->addWidget(slidersContainer);

    connect(hideBtn, &QPushButton::clicked, this, [this, slidersContainer, cell]()
    {
        cell->isVisible = !cell->isVisible;
        slidersContainer->setVisible(cell->isVisible);
        emit adjustmentChanged();
    });

    connect(removeBtn, &QPushButton::clicked, this, [this, cell]()
    {
        emit removeCellRequested(cell);
    });

    // Create one slider row per adjustable attribute
    for (auto& adjustment : cell->adjustments) {
        for (auto& attr : adjustment->attributes) {
            if (!attr.isAdjustable)
                continue;

            // Name + value row
            auto* rowWidget = new QWidget(slidersContainer);
            auto* rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(0, 0, 0, 0);

            auto* attrNameLabel = new QLabel(QString::fromStdString(attr.name), rowWidget);
            attrNameLabel->setMinimumWidth(70);

            auto* valueLabel = new QLabel(QString::number(attr.value, 'f', 2), rowWidget);
            valueLabel->setMinimumWidth(40);
            valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

            rowLayout->addWidget(attrNameLabel);
            rowLayout->addStretch();
            rowLayout->addWidget(valueLabel);
            slidersLayout->addWidget(rowWidget);

            // Slider: integer range [0, 1000] mapped linearly to [attr.min, attr.max]
            auto* slider = new QSlider(Qt::Horizontal, slidersContainer);
            slider->setRange(0, 1000);

            float range = attr.max - attr.min;
            int initVal = static_cast<int>((attr.value - attr.min) / range * 1000.f);
            slider->setValue(std::clamp(initVal, 0, 1000));

            // Connect slider change to attribute update and emit adjustmentChanged()
            AdjAttribute* attrPtr = &attr;
            connect(slider, &QSlider::valueChanged, this, [this, attrPtr, valueLabel](int v)
            {
                attrPtr->value = attrPtr->min + (v / 1000.f) * (attrPtr->max - attrPtr->min);
                valueLabel->setText(QString::number(attrPtr->value, 'f', 2));
                emit adjustmentChanged();
            });

            slidersLayout->addWidget(slider);
        }
    }
}
