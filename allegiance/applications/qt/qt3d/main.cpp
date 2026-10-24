#include <QApplication>
#include <QCommandLineParser>
#include <QTranslator>
#include <Qt3DExtras/Qt3DWindow>

#include <applications/qt/common/renderer_initializer.h>
#include <applications/qt/common/stereo_output.h>
#include <renderer/qt3d/qt3d_renderer.h>
#include <QOpenGLContext>

using Initializer = all::qt::RendererInitializer<Qt3DExtras::Qt3DWindow, all::qt3d::Qt3DRenderer>;

int main(int argc, char** argv)
{
    Q_INIT_RESOURCE(resources);

    constexpr bool useRHI = false;
    if constexpr (useRHI) {
        qputenv("QT3D_RENDERER", "rhi");
        qputenv("QSG_RHI_BACKEND", "opengl"); // could also be vulkan, metal or d3d11, d3d12.
        // Note: that stereo is not supported on RHI backends, only on opengl, d3d12 and vulkan
    } else {
        qputenv("QSG_RHI_BACKEND", "opengl");
        qputenv("QT3D_RENDERER", "opengl");
    }

    QCoreApplication::setOrganizationName(QStringLiteral("stereo3ddemo"));
    QCoreApplication::setApplicationName(QStringLiteral("stereo3ddemo"));
    if (all::qt::sideBySideChosen()) {
        // Qt gives every Wayland window a viewport the size of the window, and the desktop takes a declared
        // surface's viewport as the size of one view. A window that draws both views side by side itself
        // must have none, so Qt's viewporter is left unbound for the whole program (integer scales only).
        if (!qEnvironmentVariableIsSet("QT_WAYLAND_DISABLED_INTERFACES"))
            qputenv("QT_WAYLAND_DISABLED_INTERFACES", "wp_viewporter,wp_fractional_scale_manager_v1");
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

    std::unique_ptr<all::qt::StereoDeclaration> declaration;
    std::unique_ptr<Initializer> initializer;
    bool shown = false;

    // Makes the 3D window and its renderer for the path the user asked for and the stack offers; called again
    // when the user changes the 3D options, since a stereo context is a property of the window.
    auto start = [&]() {
        initializer.reset();
        declaration.reset();

        const bool wants3D = stereo->enabled() && !qEnvironmentVariableIsSet("DISABLE_STEREO");
        const StereoController::Path choice = stereo->path();
        const bool tryQuadBuffer = wants3D && choice != StereoController::Path::SideBySide;

        auto* renderingSurface = new Qt3DExtras::Qt3DWindow(); // Rendering Surface for 3D Renderer
        if (tryQuadBuffer) {
            // The window's format stays the one Qt3D gave it (its renderer's context is made from the same
            // default format), with the stereo buffers added
            QSurfaceFormat format = renderingSurface->requestedFormat();
            format.setStereo(true);
            renderingSurface->setFormat(format);
        }

        // Set embedded surface that holds 3d content window in the MainWindow
        mainWindow.setEmbeddedWindow(renderingSurface); // Note: renderingSurface ownership is transferred to the MainWindow
        if (!shown) {
            mainWindow.show();
            shown = true;
        }
        renderingSurface->create();

        // The window's own format says whether the stack gave a stereo context
        const bool quadBuffer = tryQuadBuffer && renderingSurface->format().stereo();
        all::StereoPath path = all::StereoPath::Off;
        if (!wants3D)
            path = all::StereoPath::Off;
        else if (quadBuffer)
            path = all::StereoPath::QuadBuffer;
        else if (choice != StereoController::Path::QuadBuffer && all::qt::sideBySidePossible())
            path = all::StereoPath::SideBySide;
        else
            path = all::StereoPath::Unavailable;
        qCDebug(all::qt::lcStereo) << "3D" << wants3D << "choice" << int(choice) << "window stereo" << renderingSurface->format().stereo()
                                   << "declaration supported" << all::qt::stereoDeclarationSupported() << "path" << all::qt::stereoPathName(path);

        mainWindow.setEmbeddedViews(all::stereoViews(path));
        if (path == all::StereoPath::SideBySide)
            declaration = std::make_unique<all::qt::StereoDeclaration>(renderingSurface, mainWindow.windowHandle());

        // Fire off Rendering
        initializer = std::make_unique<Initializer>(&mainWindow, renderingSurface, path, initialModel);

        QString description;
        switch (path) {
        case all::StereoPath::Off:
            description = QCoreApplication::translate("main", "3D off: one picture");
            break;
        case all::StereoPath::Unavailable:
            if (all::qt::stereoDeclarationSupported() && choice != StereoController::Path::SideBySide)
                description = QCoreApplication::translate("main", "No stereo context: one picture, the left eye. Choose Full side by side (declared) to draw both eyes without one");
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
    };
    QObject::connect(stereo, &StereoController::restartRequested, &mainWindow, [&]() {
        QMetaObject::invokeMethod(&mainWindow, start, Qt::QueuedConnection);
    });
    start();

    return app.exec();
}
