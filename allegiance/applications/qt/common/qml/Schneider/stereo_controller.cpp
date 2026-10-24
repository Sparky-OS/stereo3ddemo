#include "stereo_controller.h"

#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

namespace {
const auto enabledKey = QStringLiteral("stereo/enabled");
const auto pathKey = QStringLiteral("stereo/path");
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

bool StereoController::displaySettingsAvailable() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("kcmshell6")).isEmpty();
}

void StereoController::openDisplaySettings()
{
    QProcess::startDetached(QStringLiteral("kcmshell6"), { QStringLiteral("kcm_kscreen") });
}
