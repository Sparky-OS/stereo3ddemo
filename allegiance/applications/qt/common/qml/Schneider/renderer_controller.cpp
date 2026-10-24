#include "renderer_controller.h"

#include <QSettings>

namespace {
const auto engineKey = QStringLiteral("renderer/engine");
} // namespace

RendererController::RendererController(QObject* parent)
    : QObject(parent)
{
    m_engine = fromName(QSettings().value(engineKey).toString(), Engine::Quick3DVulkan);
    m_activeEngine = m_engine;
}

RendererController::Engine RendererController::engine() const
{
    return m_engine;
}

void RendererController::setEngine(Engine engine)
{
    if (m_engine == engine)
        return;
    m_engine = engine;
    QSettings().setValue(engineKey, QString::fromLatin1(name(engine)));
    Q_EMIT engineChanged(engine);
    Q_EMIT activeEngineChanged();
}

RendererController::Engine RendererController::activeEngine() const
{
    return m_activeEngine;
}

void RendererController::setActiveEngine(Engine engine)
{
    if (m_activeEngine == engine)
        return;
    m_activeEngine = engine;
    Q_EMIT activeEngineChanged();
}

bool RendererController::restartNeeded() const
{
    return m_engine != m_activeEngine;
}

const char* RendererController::name(Engine engine)
{
    switch (engine) {
    case Engine::Quick3DOpenGL:
        return "quick3d-opengl";
    case Engine::Qt3DOpenGL:
        return "qt3d-opengl";
    case Engine::Quick3DVulkan:
        break;
    }
    return "quick3d-vulkan";
}

RendererController::Engine RendererController::fromName(const QString& name, Engine fallback)
{
    if (name == QLatin1String("quick3d-vulkan"))
        return Engine::Quick3DVulkan;
    if (name == QLatin1String("quick3d-opengl"))
        return Engine::Quick3DOpenGL;
    if (name == QLatin1String("qt3d-opengl"))
        return Engine::Qt3DOpenGL;
    return fallback;
}
