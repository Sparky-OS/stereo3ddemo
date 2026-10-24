#include "stereo_controller.h"

#include <QProcess>
#include <QSettings>

#include <algorithm>
#include <QStandardPaths>

namespace {
const auto enabledKey = QStringLiteral("stereo/enabled");
const auto pathKey = QStringLiteral("stereo/path");
const auto renderResolutionKey = QStringLiteral("stereo/renderResolution");
} // namespace

StereoController::StereoController(QObject* parent)
    : QObject(parent)
{
    QSettings settings;
    m_enabled = settings.value(enabledKey, true).toBool();
    const QString path = settings.value(pathKey, QStringLiteral("automatic")).toString();
    if (path == QLatin1String("quadbuffer"))
        m_path = Path::QuadBuffer;
    else if (path == QLatin1String("sidebyside"))
        m_path = Path::SideBySide;
    // The height of an eye's picture; anything else for the 3D area's own size
    m_renderHeight = std::max(0, settings.value(renderResolutionKey).toInt());
}

bool StereoController::enabled() const
{
    return m_enabled;
}

void StereoController::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    QSettings().setValue(enabledKey, enabled);
    Q_EMIT enabledChanged(enabled);
    Q_EMIT restartRequested();
}

StereoController::Path StereoController::path() const
{
    return m_path;
}

void StereoController::setPath(Path path)
{
    if (m_path == path)
        return;
    m_path = path;
    const char* name = path == Path::QuadBuffer ? "quadbuffer" : path == Path::SideBySide ? "sidebyside" : "automatic";
    QSettings().setValue(pathKey, QString::fromLatin1(name));
    Q_EMIT pathChanged(path);
    if (m_enabled)
        Q_EMIT restartRequested();
}

QString StereoController::activePath() const
{
    return m_activePath;
}

bool StereoController::activeStereo() const
{
    return m_activeStereo;
}

void StereoController::setActivePath(const QString& description, bool stereo)
{
    if (m_activePath == description && m_activeStereo == stereo)
        return;
    m_activePath = description;
    m_activeStereo = stereo;
    Q_EMIT activePathChanged();
}

QSize StereoController::eyeSize() const
{
    return m_eyeSize;
}

void StereoController::setEyeSize(const QSize& size)
{
    if (m_eyeSize == size)
        return;
    m_eyeSize = size;
    Q_EMIT eyeSizeChanged();
}

int StereoController::renderHeight() const
{
    return m_renderHeight;
}

void StereoController::setRenderHeight(int height)
{
    const int chosen = std::max(0, height);
    if (m_renderHeight == chosen)
        return;
    m_renderHeight = chosen;
    QSettings().setValue(renderResolutionKey, chosen);
    Q_EMIT renderResolutionChanged();
}

QSize StereoController::renderSizeFor(int height, const QSize& shown)
{
    if (height <= 0 || shown.height() <= 0 || shown.width() <= 0)
        return shown;
    const int width = qRound(double(height) * shown.width() / shown.height());
    return QSize(std::max(width, 1), height);
}

QList<int> StereoController::renderHeights()
{
    return { 0, 720, 1080, 1440, 2160 };
}

QStringList StereoController::renderResolutionChoices() const
{
    QStringList choices;
    for (int height : renderHeights())
        choices << (height > 0 ? tr("%1 lines").arg(height) : tr("Same as the 3D area"));
    return choices;
}

int StereoController::renderResolutionIndex() const
{
    const int index = renderHeights().indexOf(m_renderHeight);
    return index < 0 ? 0 : index;
}

void StereoController::setRenderResolutionIndex(int index)
{
    const QList<int> heights = renderHeights();
    if (index < 0 || index >= heights.size())
        return;
    setRenderHeight(heights[index]);
}

bool StereoController::displaySettingsAvailable() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("kcmshell6")).isEmpty();
}

void StereoController::openDisplaySettings()
{
    QProcess::startDetached(QStringLiteral("kcmshell6"), { QStringLiteral("kcm_kscreen") });
}
