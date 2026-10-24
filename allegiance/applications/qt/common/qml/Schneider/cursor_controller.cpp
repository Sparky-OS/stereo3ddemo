#include "cursor_controller.h"

#include <QSettings>

CursorController::CursorController(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("cursor"));
    m_cursorType = all::CursorType(settings.value(QStringLiteral("type"), int(m_cursorType)).toInt());
    m_displayMode = all::CursorDisplayMode(settings.value(QStringLiteral("displayMode"), int(m_displayMode)).toInt());
    m_scale_factor = settings.value(QStringLiteral("scaleFactor"), m_scale_factor).toFloat();
    m_cursorHue = settings.value(QStringLiteral("hue"), m_cursorHue).toFloat();
    m_tint = QColor::fromHsv(std::clamp(m_cursorHue, 0.0f, 359.0f), 200, 255, 255);
}

void CursorController::setCursorType(CursorType cursorType)
{
    if (m_cursorType == all::CursorType(cursorType))
        return;
    m_cursorType = all::CursorType(cursorType);
    QSettings().setValue(QStringLiteral("cursor/type"), int(m_cursorType));
    cursorChanged(m_cursorType);
}

void CursorController::setScaleFactor(float scale_factor)
{
    if (m_scale_factor == scale_factor)
        return;
    m_scale_factor = scale_factor;
    QSettings().setValue(QStringLiteral("cursor/scaleFactor"), m_scale_factor);
    cursorScaleChanged(scale_factor);
}

void CursorController::setCursorTint(QColor color)
{
    if (m_tint == color)
        return;
    m_tint = color;
    cursorTintChanged(color);
}

float CursorController::scaleFactor() const
{
    return m_scale_factor;
}

CursorController::CursorType CursorController::cursor() const
{
    return CursorType(m_cursorType);
}

QColor CursorController::cursorTint() const
{
    return m_tint;
}

void CursorController::setHue(float hue)
{
    if (m_cursorHue == hue)
        return;

    m_cursorHue = hue;
    QSettings().setValue(QStringLiteral("cursor/hue"), m_cursorHue);
    cursorHueChanged(m_cursorHue);
    setCursorTint(QColor::fromHsv(std::clamp(hue, 0.0f, 359.0f), 200, 255, 255));
}

float CursorController::hue()
{
    return m_cursorHue;
}

CursorController::CursorDisplayMode CursorController::displayMode() const
{
    return CursorDisplayMode(m_displayMode);
}

void CursorController::setDisplayMode(CursorDisplayMode displayMode)
{
    if (m_displayMode == all::CursorDisplayMode(displayMode))
        return;

    m_displayMode = all::CursorDisplayMode(displayMode);
    QSettings().setValue(QStringLiteral("cursor/displayMode"), int(m_displayMode));
    displayModeChanged(m_displayMode);
}

void CursorController::cycleDisplayMode()
{
    // both -> 3d cursor
    if (m_displayMode == all::CursorDisplayMode::Both) {
        setDisplayMode(CursorDisplayMode::ThreeDimensionalOnly);
        return;
    }

    // 3d cursor -> default
    if (m_displayMode == all::CursorDisplayMode::ThreeDimensionalOnly) {
        setDisplayMode(CursorDisplayMode::SystemCursorOnly);
        return;
    }

    // default -> both
    setDisplayMode(CursorDisplayMode::Both);
}
