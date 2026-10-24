#include "camera_controller.h"

#include <shared/stereo_frustum.h>

#include <QSettings>

namespace {
template<typename T>
void save(const char* key, const T& value)
{
    QSettings().setValue(QStringLiteral("camera/") + QLatin1String(key), value);
}
} // namespace

CameraController::CameraController(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("camera"));
    m_displayMode = DisplayMode(settings.value(QStringLiteral("displayMode"), int(m_displayMode)).toInt());
    m_stereoMode = StereoMode(settings.value(QStringLiteral("stereoMode"), int(m_stereoMode)).toInt());
    m_eyeDistance = settings.value(QStringLiteral("eyeDistance"), m_eyeDistance).toFloat();
    m_flipped = settings.value(QStringLiteral("flipped"), m_flipped).toBool();
    m_autoFocus = settings.value(QStringLiteral("autoFocus"), m_autoFocus).toBool();
    m_showAutoFocusArea = settings.value(QStringLiteral("showAutoFocusArea"), m_showAutoFocusArea).toBool();
    m_focusDistance = settings.value(QStringLiteral("focusDistance"), m_focusDistance).toFloat();
    m_popOut = settings.value(QStringLiteral("popOut"), m_popOut).toFloat();
    m_fov = settings.value(QStringLiteral("fov"), m_fov).toFloat();
    m_fovByPhysicalDim = settings.value(QStringLiteral("fovByPhysicalDim"), m_fovByPhysicalDim).toBool();
    m_screenHeight = settings.value(QStringLiteral("screenHeight"), m_screenHeight).toFloat();
    m_viewerDistance = settings.value(QStringLiteral("viewerDistance"), m_viewerDistance).toFloat();
    m_separationBasedOnFocusDistance = settings.value(QStringLiteral("separationBasedOnFocusDistance"), m_separationBasedOnFocusDistance).toBool();
    m_showFocusPlane = settings.value(QStringLiteral("showFocusPlane"), m_showFocusPlane).toBool();
    m_separationBasedOnFocusDistanceDivider = settings.value(QStringLiteral("separationBasedOnFocusDistanceDivider"), m_separationBasedOnFocusDistanceDivider).toInt();
    settings.endGroup();

    QObject::connect(this, &CameraController::screenHeightChanged, this, &CameraController::updateFovFromDims);
    QObject::connect(this, &CameraController::viewerDistanceChanged, this, &CameraController::updateFovFromDims);
    QObject::connect(this, &CameraController::fovByPhysicalDimChanged, [this](bool computeFovFromDims) {
        if (computeFovFromDims)
            updateFovFromDims();
    });
}

void CameraController::setFov(float fov)
{
    if (m_fov == fov)
        return;
    m_fov = fov;
    save("fov", fov);
    Q_EMIT fovChanged(fov);
}

float CameraController::fov() const
{
    return m_fov;
}

void CameraController::setEyeDistance(float eyeDistance)
{
    const float safeEyeDistance = std::clamp(eyeDistance, CameraController::MinEyeDistance, CameraController::MaxEyeDistance);
    if (m_eyeDistance == safeEyeDistance)
        return;
    m_eyeDistance = safeEyeDistance;
    save("eyeDistance", m_eyeDistance);
    Q_EMIT eyeDistanceChanged(m_eyeDistance);
}

float CameraController::eyeDistance() const
{
    return m_eyeDistance;
}

bool CameraController::separationBasedOnFocusDistance() const
{
    return m_separationBasedOnFocusDistance;
}

void CameraController::setSeparationBasedOnFocusDistance(bool newSeparationBasedOnFocusDistance)
{
    if (m_separationBasedOnFocusDistance == newSeparationBasedOnFocusDistance)
        return;
    m_separationBasedOnFocusDistance = newSeparationBasedOnFocusDistance;
    save("separationBasedOnFocusDistance", m_separationBasedOnFocusDistance);
    Q_EMIT separationBasedOnFocusDistanceChanged(m_separationBasedOnFocusDistance);
}

void CameraController::setFocusDistance(float focusDistance)
{
    const float safeFocusDistance = std::clamp(focusDistance, CameraController::MinFocusDistance, CameraController::MaxFocusDistance);

    if (qFuzzyCompare(m_focusDistance, safeFocusDistance))
        return;
    m_focusDistance = safeFocusDistance;
    save("focusDistance", m_focusDistance);
    Q_EMIT focusDistanceChanged(m_focusDistance);
}

float CameraController::focusDistance() const
{
    return m_focusDistance;
}

void CameraController::setFlipped(bool flipped)
{
    if (m_flipped == flipped)
        return;
    m_flipped = flipped;
    save("flipped", flipped);
    Q_EMIT flippedChanged(flipped);
}

bool CameraController::flipped() const
{
    return m_flipped;
}

bool CameraController::autoFocus() const
{
    return m_autoFocus;
}

void CameraController::setAutoFocus(bool newAutoFocus)
{
    if (m_autoFocus == newAutoFocus)
        return;
    m_autoFocus = newAutoFocus;
    save("autoFocus", m_autoFocus);
    Q_EMIT autoFocusChanged(m_autoFocus);
}

bool CameraController::showAutoFocusArea() const
{
    return m_showAutoFocusArea;
}

void CameraController::setShowAutoFocusArea(bool newShowAutoFocusArea)
{
    if (m_showAutoFocusArea == newShowAutoFocusArea)
        return;
    m_showAutoFocusArea = newShowAutoFocusArea;
    save("showAutoFocusArea", m_showAutoFocusArea);
    Q_EMIT showAutoFocusAreaChanged(m_showAutoFocusArea);
}

float CameraController::screenHeight() const
{
    return m_screenHeight;
}

void CameraController::setScreenHeight(float newScreenHeight)
{
    if (m_screenHeight == newScreenHeight)
        return;
    m_screenHeight = newScreenHeight;
    save("screenHeight", m_screenHeight);
    Q_EMIT screenHeightChanged(m_screenHeight);
}

float CameraController::viewerDistance() const
{
    return m_viewerDistance;
}

void CameraController::setViewerDistance(float newViewerDistance)
{
    if (m_viewerDistance == newViewerDistance)
        return;
    m_viewerDistance = newViewerDistance;
    save("viewerDistance", m_viewerDistance);
    Q_EMIT viewerDistanceChanged(newViewerDistance);
}

float CameraController::popOut() const
{
    return m_popOut;
}

void CameraController::setPopOut(float newPopOut)
{
    const float safePopOut = std::clamp(newPopOut, -100.0f, 100.0f);
    if (m_popOut == safePopOut)
        return;
    m_popOut = safePopOut;
    save("popOut", m_popOut);
    Q_EMIT popOutChanged(m_popOut);
}

void CameraController::setStereoMode(StereoMode newStereoMode)
{
    if (newStereoMode == m_stereoMode)
        return;
    m_stereoMode = newStereoMode;
    save("stereoMode", int(newStereoMode));
    Q_EMIT stereoModeChanged(newStereoMode);
}
CameraController::StereoMode CameraController::stereoMode() const
{
    return m_stereoMode;
}

void CameraController::setDisplayMode(DisplayMode mode)
{
    if (m_displayMode == mode)
        return;
    m_displayMode = mode;
    save("displayMode", int(mode));
    Q_EMIT displayModeChanged(mode);
}

CameraController::DisplayMode CameraController::displayMode() const
{
    return m_displayMode;
}

bool CameraController::fovByPhysicalDim() const
{
    return m_fovByPhysicalDim;
}

void CameraController::setFovByPhysicalDim(bool newFovByPhysicalDim)
{
    if (m_fovByPhysicalDim == newFovByPhysicalDim)
        return;
    m_fovByPhysicalDim = newFovByPhysicalDim;
    save("fovByPhysicalDim", m_fovByPhysicalDim);
    Q_EMIT fovByPhysicalDimChanged(m_fovByPhysicalDim);
}

void CameraController::updateFovFromDims()
{
    setFov(all::fovFromPhysical(screenHeight(), viewerDistance()));
}

bool CameraController::showFocusPlane() const
{
    return m_showFocusPlane;
}

void CameraController::setShowFocusPlane(bool newShowFocusPlane)
{
    if (m_showFocusPlane == newShowFocusPlane)
        return;
    m_showFocusPlane = newShowFocusPlane;
    save("showFocusPlane", m_showFocusPlane);
    Q_EMIT showFocusPlaneChanged(m_showFocusPlane);
}

int CameraController::separationBasedOnFocusDistanceDivider() const
{
    return m_separationBasedOnFocusDistanceDivider;
}

void CameraController::setSeparationBasedOnFocusDistanceDivider(int newSeparationBasedOnFocusDistanceDivider)
{
    if (m_separationBasedOnFocusDistanceDivider == newSeparationBasedOnFocusDistanceDivider)
        return;
    m_separationBasedOnFocusDistanceDivider = newSeparationBasedOnFocusDistanceDivider;
    save("separationBasedOnFocusDistanceDivider", m_separationBasedOnFocusDistanceDivider);
    Q_EMIT separationBasedOnFocusDistanceDividerChanged(m_separationBasedOnFocusDistanceDivider);
}

float CameraController::eyeDistanceDefaultValue() const
{
    return ms_eyeDistanceDefaultValue;
}

int CameraController::separationBasedOnFocusDistanceDividerDefaultValue() const
{
    return ms_separationBasedOnFocusDistanceDividerDefaultValue;
}

float CameraController::focusDistanceDefaultValue() const
{
    return ms_focusDistanceDefaultValue;
}

float CameraController::popOutDefaultValue() const
{
    return ms_popOutDefaultValue;
}

float CameraController::screenHeightDefaultValue() const
{
    return ms_screenHeightDefaultValue;
}

float CameraController::viewerDistanceDefaultValue() const
{
    return ms_viewerDistanceDefaultValue;
}

float CameraController::fovDefaultValue() const
{
    return ms_fovDefaultValue;
}
