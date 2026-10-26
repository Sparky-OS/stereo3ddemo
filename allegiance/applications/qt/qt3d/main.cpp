#include <QApplication>
#include <QCommandLineParser>
#include <QQuickWindow>
#include <QPointer>
#include <QSGRendererInterface>
#include <QSettings>
#include <QSurfaceFormat>
#include <QTranslator>
#include <Qt3DExtras/Qt3DWindow>

#include <applications/qt/common/renderer_initializer.h>
#include <applications/qt/common/stereo_output.h>
#include <applications/qt/common/qml/Schneider/renderer_controller.h>
#include <renderer/qt3d/qt3d_renderer.h>
#ifdef ALLEGIANCE_QUICK3D
#include <renderer/quick3d/quick3d_renderer.h>
#include <QtQml/qqmlextensionplugin.h>
#include <rhi/qrhi.h>
// The renderer's QML module is static
Q_IMPORT_QML_PLUGIN(Stereo3D_Quick3DPlugin)
#endif
#include <QOpenGLContext>
#include <functional>

// The two renderers of the stereo version in one program: the original Qt 3D renderer (OpenGL) and the phase 2
// Qt Quick 3D renderer (Vulkan or OpenGL, each eye drawn by the edition's Qt Quick). The choice is under Advanced.
using Qt3DInitializer = all::qt::RendererInitializer<Qt3DExtras::Qt3DWindow, all::qt3d::Qt3DRenderer>;
#ifdef ALLEGIANCE_QUICK3D
using Quick3DInitializer = all::qt::RendererInitializer<QQuickWindow, all::quick3d::Quick3DRenderer>;
#endif

namespace {

// Which renderer this start uses: the setting, unless the build lacks it
RendererController::Engine engineForThisStart()
{
    const auto chosen = RendererController::fromName(QSettings().value(QStringLiteral("renderer/engine")).toString(), RendererController::Engine::Quick3DVulkan);
#ifndef ALLEGIANCE_QUICK3D
    Q_UNUSED(chosen);
    return RendererController::Engine::Qt3DOpenGL;
#else
    return chosen;
#endif
}

} // namespace

int main(int argc, char** argv)
{
    Q_INIT_RESOURCE(resources);

    QCoreApplication::setOrganizationName(QStringLiteral("stereo3ddemo"));
    QCoreApplication::setApplicationName(QStringLiteral("stereo3ddemo"));

    // The graphics API is Qt Quick's for the whole program and is fixed before the first window
    const RendererController::Engine engine = engineForThisStart();
    if (RendererController::usesQt3D(engine)) {
        qputenv("QSG_RHI_BACKEND", "opengl");
        qputenv("QT3D_RENDERER", "opengl");
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    } else if (!qEnvironmentVariableIsSet("QSG_RHI_BACKEND")) {
        QQuickWindow::setGraphicsApi(RendererController::usesVulkan(engine) ? QSGRendererInterface::Vulkan : QSGRendererInterface::OpenGL);
    }

    QApplication app(argc, argv);
    QApplication::setApplicationDisplayName(QStringLiteral("Stereo Demonstrator - " ALLEGIANCE_BUILD_STR));
    QApplication::setApplicationVersion(QStringLiteral(ALLEGIANCE_PROJECT_VERSION));

#ifdef ALLEGIANCE_TRANSLATIONS
    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("stereo3ddemo"), QStringLiteral("_"), QStringLiteral(":/i18n")))
        app.installTranslator(&translator);
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Stereo Demonstrator: a model viewer for stereo displays, with a 3D cursor"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("model"), QCoreApplication::translate("main", "A model file to open (.obj, .fbx, .gltf, .glb)"), QStringLiteral("[model]"));
    parser.process(app);
    const std::filesystem::path initialModel = parser.positionalArguments().isEmpty() ? std::filesystem::path() : std::filesystem::path(parser.positionalArguments().first().toStdString());

    all::qt::initStereoDeclaration();

    // The application's default format stays 2D: a stereo default format would make every window stereo, the
    // side menu's included. The stereo format goes on the 3D window alone, below.
    {
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setSamples(8);
        QSurfaceFormat::setDefaultFormat(format);
    }

    all::qt::MainWindow mainWindow({ 1920, 1080 }); // Main Application Window
    StereoController* stereo = mainWindow.sideMenu()->stereoController();
    RendererController* rendererChoice = mainWindow.sideMenu()->rendererController();
    rendererChoice->setActiveEngine(engine);
    qCDebug(all::qt::lcStereo) << "renderer" << RendererController::name(engine) << "graphics api" << int(QQuickWindow::graphicsApi());

    std::unique_ptr<all::qt::StereoDeclaration> declaration;
    std::unique_ptr<Qt3DInitializer> qt3dInitializer;
#ifdef ALLEGIANCE_QUICK3D
    std::unique_ptr<Quick3DInitializer> quick3dInitializer;
#endif
    bool shown = false;
    bool quadBufferUnavailable = false;
    QMetaObject::Connection renderSizeConnection;
    QMetaObject::Connection stereoCapabilityConnection;

    // Makes the 3D window and its renderer for the path the user asked for and the stack offers; called again
    // when the user changes the 3D options, since a stereo context is a property of the window.
    std::function<void()> start;
    start = [&]() {
        QObject::disconnect(stereoCapabilityConnection);
        qt3dInitializer.reset();
#ifdef ALLEGIANCE_QUICK3D
        quick3dInitializer.reset();
#endif
        declaration.reset();
        QObject::disconnect(renderSizeConnection);

        const bool wants3D = stereo->enabled() && !qEnvironmentVariableIsSet("DISABLE_STEREO");
        const StereoController::Path choice = stereo->path();
        const bool tryQuadBuffer = wants3D && choice != StereoController::Path::SideBySide && !quadBufferUnavailable;

        QWindow* renderingSurface = nullptr; // Rendering Surface for 3D Renderer
        if (RendererController::usesQt3D(engine)) {
            renderingSurface = new Qt3DExtras::Qt3DWindow();
        } else {
#ifdef ALLEGIANCE_QUICK3D
            auto* quickWindow = new QQuickWindow();
            quickWindow->setColor(QColor(0x48, 0x53, 0x6a));
            // The views antialias their own pictures; the window's own buffers stay plain, so that each eye's
            // target is the platform's (a multisample resolve would land in the left one)
            QSurfaceFormat plain = quickWindow->requestedFormat();
            plain.setSamples(0);
            quickWindow->setFormat(plain);
            renderingSurface = quickWindow;
#endif
        }
        if (!renderingSurface)
            return;
        if (tryQuadBuffer) {
            // The window's format stays the one its renderer gave it, with the stereo buffers added: on Vulkan a
            // two-layer swapchain, on OpenGL the platform's two views
            QSurfaceFormat format = renderingSurface->requestedFormat();
            format.setStereo(true);
            renderingSurface->setFormat(format);
            // A Qt Quick window is drawn once per eye by the edition's Qt Quick (its surface gets its two views
            // when it is created, so this comes first); its pair is 3D content, two cameras, not a 2D picture
            // with depth (qt6-base +stereo3d11 puts no desktop-depth mark on it)
            if (!RendererController::usesQt3D(engine)) {
                renderingSurface->setProperty("_q_stereo_depth", true);
                renderingSurface->setProperty("_q_stereo_content", true);
            }
        }

        // Set embedded surface that holds 3d content window in the MainWindow
        mainWindow.setEmbeddedWindow(renderingSurface); // Note: renderingSurface ownership is transferred to the MainWindow
        if (!shown) {
            mainWindow.show();
            shown = true;
        }
        renderingSurface->create();

        // The window's own format says whether the stack gave a stereo surface
        const bool quadBuffer = tryQuadBuffer && renderingSurface->format().stereo();
        all::StereoPath path = all::StereoPath::Off;
        if (!wants3D)
            path = all::StereoPath::Off;
        else if (quadBuffer)
            path = all::StereoPath::QuadBuffer;
        else if (choice != StereoController::Path::QuadBuffer && all::qt::stereoDeclarationSupported())
            path = all::StereoPath::SideBySide;
        else
            path = all::StereoPath::Unavailable;
        qCDebug(all::qt::lcStereo) << "3D" << wants3D << "choice" << int(choice) << "window stereo" << renderingSurface->format().stereo()
                                   << "declaration supported" << all::qt::stereoDeclarationSupported() << "path" << all::qt::stereoPathName(path)
                                   << "renderer" << RendererController::name(engine);

        mainWindow.setEmbeddedViews(all::stereoViews(path));
        // The render resolution per eye: the Qt Quick 3D renderer draws each eye at it; the Qt 3D renderer follows
        // the 3D area
        auto applyRenderSize = [&mainWindow, stereo, engine, path]() {
            mainWindow.setEmbeddedRenderHeight(!RendererController::usesQt3D(engine) && all::stereoDraws(path) ? stereo->renderHeight() : 0);
        };
        applyRenderSize();
        renderSizeConnection = QObject::connect(stereo, &StereoController::renderResolutionChanged, &mainWindow, applyRenderSize);
        if (path == all::StereoPath::SideBySide)
            declaration = std::make_unique<all::qt::StereoDeclaration>(renderingSurface, mainWindow.windowHandle());

        // Fire off Rendering
        if (RendererController::usesQt3D(engine)) {
            qt3dInitializer = std::make_unique<Qt3DInitializer>(&mainWindow, static_cast<Qt3DExtras::Qt3DWindow*>(renderingSurface), path, initialModel);
        } else {
#ifdef ALLEGIANCE_QUICK3D
            quick3dInitializer = std::make_unique<Quick3DInitializer>(&mainWindow, static_cast<QQuickWindow*>(renderingSurface), path, initialModel);
#endif
        }

        QString description;
        switch (path) {
        case all::StereoPath::Off:
            description = QCoreApplication::translate("main", "3D off: one picture");
            break;
        case all::StereoPath::Unavailable:
            if (all::qt::stereoDeclarationSupported())
                description = QCoreApplication::translate("main", "No stereo context: one picture, the left eye. Choose Automatic or Two views side by side (declared) to draw both eyes without one");
            else
                description = QCoreApplication::translate("main", "No stereo path on this desktop: one picture, the left eye");
            break;
        case all::StereoPath::QuadBuffer:
            description = QCoreApplication::translate("main", "Quad buffer: the display stack packs the two eyes");
            break;
        case all::StereoPath::SideBySide:
            description = QCoreApplication::translate("main", "Full side by side, declared to the desktop");
            break;
        }
        stereo->setActivePath(description, all::stereoDraws(path));

#ifdef ALLEGIANCE_QUICK3D
        if (path == all::StereoPath::QuadBuffer && !RendererController::usesQt3D(engine)) {
            auto* window = static_cast<QQuickWindow*>(renderingSurface);
            // QWindow's format can retain the stereo request even when the RHI only has one target.
            // Inspect the created swapchain on its render thread, then recreate on the GUI thread.
            stereoCapabilityConnection = QObject::connect(window, &QQuickWindow::afterFrameEnd, window,
                    [&, window, checked = false]() mutable {
                if (checked)
                    return;
                auto* swapchain = static_cast<QRhiSwapChain*>(window->rendererInterface()->getResource(
                        window, QSGRendererInterface::RhiSwapchainResource));
                if (!swapchain)
                    return;
                checked = true;
                const bool hasRightTarget = swapchain->currentFrameRenderTarget(QRhiSwapChain::LeftBuffer)
                        != swapchain->currentFrameRenderTarget(QRhiSwapChain::RightBuffer);
                qCDebug(all::qt::lcStereo) << "swapchain has distinct eye targets" << hasRightTarget;
                if (!hasRightTarget) {
                    QMetaObject::invokeMethod(&mainWindow, [&, window = QPointer<QQuickWindow>(window)] {
                        if (!window || mainWindow.embeddedWindow() != window)
                            return;
                        quadBufferUnavailable = true;
                        start();
                    }, Qt::QueuedConnection);
                }
            }, Qt::DirectConnection);
        }
#endif
    };
    QObject::connect(stereo, &StereoController::restartRequested, &mainWindow, [&]() {
        QMetaObject::invokeMethod(&mainWindow, start, Qt::QueuedConnection);
    });
    start();

    const int result = app.exec();
    QObject::disconnect(stereoCapabilityConnection);
    return result;
}
