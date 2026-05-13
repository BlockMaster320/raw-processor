#include <QString>

const QString appBackgroundColor = QStringLiteral("#272727");   // background
const QString appPanelBackgroundColor = QStringLiteral("#1b1b1b");

const QString appSurfaceColor = QStringLiteral("#414141");      // buttons
const QString appSurfaceHoverColor = QStringLiteral("#505050");
const QString appSurfacePressedColor = QStringLiteral("#2f2f2f");

const QString appTextColor = QStringLiteral("#dfe4ea");         // text
const QString appMutedTextColor = QStringLiteral("#6f7f8d");
const QString appDisabledTextColor = QStringLiteral("#7f8c8d");

const QString appSelectedColor = QStringLiteral("#f1f2f6");     // selected state (e.g. active cell, selected preset)
const QString appSelectedTextColor = QStringLiteral("#3b3b3b");

const QString appSwitchDisabledCheckedColor = QStringLiteral("#7c7c7c");    // disabled switch checked state (e.g. enabled switch when cell is disabled)

const QString appCompactSwitchOffCheckedColor = QStringLiteral("#8d8d8d");
const QString appCompactSwitchOnCheckedColor = QStringLiteral("#ffffff");
const QString appCompactSwitchDisabledOffCheckedColor = QStringLiteral("#636363");
const QString appCompactSwitchDisabledOnCheckedColor = QStringLiteral("#8e949b");

const QString appActiveHighlightColor = QStringLiteral("#5a9fd4");

const QString baseButtonStyle = QStringLiteral(
    "QPushButton {"
    " border: none;"
    " border-radius: 10px;"
    " padding: 8px 14px;"
    " background-color: %1;"
    " color: %2;"
    " font-weight: 600;"
    " }"
    "QPushButton:hover { background-color: %3; }"
    "QPushButton:pressed { background-color: %4; }"
    "QPushButton:disabled { background-color: %4; color: %5; }"
).arg(appSurfaceColor, appTextColor, appSurfaceHoverColor, appSurfacePressedColor, appDisabledTextColor);

const QString switchStyle = QStringLiteral(
    "QWidget {"
    " background-color: %1;"
    " border-radius: 12px;"
    " }"
    "QPushButton {"
    " border: none;"
    " border-radius: 10px;"
    " padding: 6px 14px;"
    " background: transparent;"
    " color: %2;"
    " font-weight: 600;"
    " }"
    "QPushButton:checked {"
    " background-color: %3;"
    " color: %4;"
    " }"
    "QPushButton:disabled { color: %5; }"
    "QPushButton:disabled:checked {"
    " background-color: %6;"
    " color: %5;"
    " }"
).arg(appSurfaceColor, appTextColor, appSelectedColor, appSelectedTextColor, appMutedTextColor, appSwitchDisabledCheckedColor);

const QString applyControlsPanelStyle = QStringLiteral(
    "QWidget#applyControlsWidget {"
    " background-color: %1;"
    " border-radius: 12px;"
    " }"
    "QLabel#applyControlsTitle {"
    " background-color: %1;"
    " color: %2;"
    " font-weight: 700;"
    " padding: 2px 0;"
    " }"
).arg(appPanelBackgroundColor, appTextColor);

const QString compactSwitchStyle = QStringLiteral(
    "QWidget#autoPresetSwitchWidget {"
    " background-color: %1;"
    " border-radius: 9px;"
    " }"
    "QPushButton#autoPresetOffButton, QPushButton#autoPresetOnButton {"
    " border: none;"
    " border-radius: 7px;"
    " background: transparent;"
    " }"
    "QPushButton#autoPresetOffButton:checked {"
    " background-color: %2;"
    " }"
    "QPushButton#autoPresetOnButton:checked {"
    " background-color: %3;"
    " }"
    "QPushButton#autoPresetOffButton:disabled, QPushButton#autoPresetOnButton:disabled {"
    " background: transparent;"
    " }"
    "QPushButton#autoPresetOffButton:disabled:checked {"
    " background-color: %4;"
    " }"
    "QPushButton#autoPresetOnButton:disabled:checked {"
    " background-color: %5;"
    " }"
).arg(appSurfaceColor, appCompactSwitchOffCheckedColor, appCompactSwitchOnCheckedColor, appCompactSwitchDisabledOffCheckedColor, appCompactSwitchDisabledOnCheckedColor);