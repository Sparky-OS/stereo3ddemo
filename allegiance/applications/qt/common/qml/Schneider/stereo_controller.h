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
    // The render resolution per eye, as the height of an eye's picture: each eye is drawn at that height, with the
    // width the 3D area's aspect gives (an eye is never stretched), and the desktop resizes it once for the screen;
    // 0 follows the 3D area
    Q_PROPERTY(int renderHeight READ renderHeight WRITE setRenderHeight NOTIFY renderResolutionChanged)
    Q_PROPERTY(QStringList renderResolutionChoices READ renderResolutionChoices CONSTANT)
    Q_PROPERTY(int renderResolutionIndex READ renderResolutionIndex WRITE setRenderResolutionIndex NOTIFY renderResolutionChanged)
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
    int renderHeight() const;
    void setRenderHeight(int height);
    // The size of an eye's picture for a shown size: the chosen height with the shown aspect, or the shown size
    static QSize renderSizeFor(int height, const QSize& shown);
    // The choices offered: the 3D area's own size first, then the common heights
    QStringList renderResolutionChoices() const;
    int renderResolutionIndex() const;
    void setRenderResolutionIndex(int index);
    static QList<int> renderHeights();

    bool displaySettingsAvailable() const;
    Q_INVOKABLE void openDisplaySettings();

Q_SIGNALS:
    void enabledChanged(bool);
    void pathChanged(Path);
    void activePathChanged();
    void eyeSizeChanged();
    void renderResolutionChanged();
    // The chosen path needs another window: the application makes it.
    void restartRequested();

private:
    bool m_enabled = true;
    Path m_path = Path::Automatic;
    QString m_activePath;
    bool m_activeStereo = false;
    QSize m_eyeSize;
    int m_renderHeight = 0;
};
