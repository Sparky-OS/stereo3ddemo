#pragma once
#include <QObject>
#include <QtQml/qqmlregistration.h>

// The renderer that draws the 3D area: Qt Quick 3D on Vulkan (the default where the stack allows), Qt Quick 3D on
// OpenGL, or the Qt 3D renderer of the original demonstrator. Each gives a different picture only where the stack
// differs, so the choice lives under Advanced. The graphics API is fixed when the program starts (Qt Quick's), so a
// change applies at the next start.
class RendererController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Engine engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(Engine activeEngine READ activeEngine NOTIFY activeEngineChanged)
    Q_PROPERTY(bool restartNeeded READ restartNeeded NOTIFY activeEngineChanged)
    QML_SINGLETON
    QML_NAMED_ELEMENT(Renderer)
public:
    enum class Engine {
        Quick3DVulkan,
        Quick3DOpenGL,
        Qt3DOpenGL
    };
    Q_ENUM(Engine)

    explicit RendererController(QObject* parent = nullptr);

    Engine engine() const;
    void setEngine(Engine engine);

    // The engine the running program was started with
    Engine activeEngine() const;
    void setActiveEngine(Engine engine);
    bool restartNeeded() const;

    static const char* name(Engine engine);
    static Engine fromName(const QString& name, Engine fallback);
    static bool usesVulkan(Engine engine) { return engine == Engine::Quick3DVulkan; }
    static bool usesQt3D(Engine engine) { return engine == Engine::Qt3DOpenGL; }

Q_SIGNALS:
    void engineChanged(Engine);
    void activeEngineChanged();

private:
    Engine m_engine = Engine::Quick3DVulkan;
    Engine m_activeEngine = Engine::Quick3DVulkan;
};
