#include "stereo_controller.h"

#include <QProcess>
#include <QSettings>
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
    // "WxH", or anything else for the 3D area's own size
    const QStringList resolution = settings.value(renderResolutionKey).toString().split(QLatin1Char('x'));
    if (resolution.size() == 2)
        m_renderResolution = QSize(resolution[0].toInt(), resolution[1].toInt());
    if (m_renderResolution.width() <= 0 || m_renderResolution.height() <= 0)
        m_renderResolution = QSize();
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

QSize StereoController::renderResolution() const
{
    return m_renderResolution;
}

void StereoController::setRenderResolution(const QSize& size)
{
    const QSize chosen = (size.width() > 0 && size.height() > 0) ? size : QSize();
    if (m_renderResolution == chosen)
        return;
    m_renderResolution = chosen;
    QSettings().setValue(renderResolutionKey, chosen.isValid() ? QStringLiteral("%1x%2").arg(chosen.width()).arg(chosen.height()) : QStringLiteral("area"));
    Q_EMIT renderResolutionChanged();
}

QList<QSize> StereoController::renderResolutionSizes()
{
    return { QSize(), QSize(1280, 720), QSize(1920, 1080), QSize(2560, 1440), QSize(3840, 2160) };
}

QStringList StereoController::renderResolutionChoices() const
{
    QStringList choices;
    for (const QSize& size : renderResolutionSizes())
        choices << (size.isValid() ? QStringLiteral("%1 x %2").arg(size.width()).arg(size.height()) : tr("Same as the 3D area"));
    return choices;
}

int StereoController::renderResolutionIndex() const
{
    const int index = renderResolutionSizes().indexOf(m_renderResolution);
    return index < 0 ? 0 : index;
}

void StereoController::setRenderResolutionIndex(int index)
{
    const QList<QSize> sizes = renderResolutionSizes();
    if (index < 0 || index >= sizes.size())
        return;
    setRenderResolution(sizes[index]);
}

bool StereoController::displaySettingsAvailable() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("kcmshell6")).isEmpty();
}

void StereoController::openDisplaySettings()
{
    QProcess::startDetached(QStringLiteral("kcmshell6"), { QStringLiteral("kcm_kscreen") });
}
