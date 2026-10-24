#pragma once

#include <QObject>
#include <QSize>
#include <QtQml/qqmlregistration.h>

// The program's 3D options: whether 3D is on, which path to the screen, and what is active. The desktop does
// the display work (every screen format, the eye swap per output), so there is nothing else here.
class StereoController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(Path path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString activePath READ activePath NOTIFY activePathChanged)
    Q_PROPERTY(bool activeStereo READ activeStereo NOTIFY activePathChanged)
    Q_PROPERTY(QSize eyeSize READ eyeSize NOTIFY eyeSizeChanged)
    Q_PROPERTY(bool displaySettingsAvailable READ displaySettingsAvailable CONSTANT)

    QML_SINGLETON
    QML_NAMED_ELEMENT(Stereo)

public:
    enum class Path {
        Automatic,
        QuadBuffer,
        SideBySide
    };
    Q_ENUM(Path)

    explicit StereoController(QObject* parent = nullptr);

    bool enabled() const;
    void setEnabled(bool enabled);

    Path path() const;
    void setPath(Path path);

    QString activePath() const;
    bool activeStereo() const;
    void setActivePath(const QString& description, bool stereo);

    QSize eyeSize() const;
    void setEyeSize(const QSize& size);

    bool displaySettingsAvailable() const;
    Q_INVOKABLE void openDisplaySettings();

Q_SIGNALS:
    void enabledChanged(bool);
    void pathChanged(Path);
    void activePathChanged();
    void eyeSizeChanged();
    // The chosen path needs another window: the application makes it.
    void restartRequested();

private:
    bool m_enabled = true;
    Path m_path = Path::Automatic;
    QString m_activePath;
    bool m_activeStereo = false;
    QSize m_eyeSize;
};
