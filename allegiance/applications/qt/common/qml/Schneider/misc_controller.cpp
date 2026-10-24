#include "misc_controller.h"

#include <QSettings>

MiscController::MiscController(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    m_frustumViewEnabled = settings.value(QStringLiteral("misc/frustumViewEnabled"), m_frustumViewEnabled).toBool();
    m_wireframeEnabled = settings.value(QStringLiteral("misc/wireframeEnabled"), m_wireframeEnabled).toBool();
}

bool MiscController::frustumViewEnabled() const
{
    return m_frustumViewEnabled;
}

void MiscController::setFrustumViewEnabled(bool newFrustumViewEnabled)
{
    if (newFrustumViewEnabled == m_frustumViewEnabled)
        return;
    m_frustumViewEnabled = newFrustumViewEnabled;
    QSettings().setValue(QStringLiteral("misc/frustumViewEnabled"), m_frustumViewEnabled);
    Q_EMIT frustumViewEnabledChanged(newFrustumViewEnabled);
}

bool MiscController::wireframeEnabled() const
{
    return m_wireframeEnabled;
}

void MiscController::setWireframeEnabled(bool newWireframeEnabled)
{
    if (m_wireframeEnabled == newWireframeEnabled)
        return;
    m_wireframeEnabled = newWireframeEnabled;
    QSettings().setValue(QStringLiteral("misc/wireframeEnabled"), m_wireframeEnabled);
    Q_EMIT wireframeEnabledChanged(m_wireframeEnabled);
}
