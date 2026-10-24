#include <QtTest/QtTest>

#include <renderer/quick3d/quick3d_renderer.h>
#include <shared/stereo_camera.h>
#include <shared/stereo_frustum.h>
#include <applications/qt/common/main_window.h>
#include <applications/qt/common/renderer_initializer.h>

// Qt Quick 3D's private headers spell the Qt keywords, which the renderer's build turns off
#define signals public
#define slots
#include <QtQuick3D/private/qquick3dcamera_p.h>
#undef signals
#undef slots

#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QSettings>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>

// The renderer's QML module is static
Q_IMPORT_QML_PLUGIN(Stereo3D_Quick3DPlugin)

#include <glm/ext/matrix_clip_space.hpp>

#include <any>
#include <cmath>

// The Qt Quick 3D renderer's cameras: the left and right cameras are the shared off-axis maths (the frusta of the
// stereo_frustum test), the centre camera is symmetric between them, and the view shows the eye the window is
// drawing on the quad-buffer path, or one eye per view on the declared pair path.
class Quick3DRendererTest : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settingsDir;

    static QMatrix4x4 fromGlm(const glm::mat4& m)
    {
        QMatrix4x4 q;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                q(r, c) = m[c][r];
        return q;
    }

    static bool close(const QMatrix4x4& a, const QMatrix4x4& b, float eps = 1e-4f)
    {
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                if (std::fabs(a(r, c) - b(r, c)) > eps * std::max(1.0f, std::fabs(b(r, c))))
                    return false;
        return true;
    }

    struct Scene {
        QQuickWindow window;
        all::StereoCamera camera;
        std::unique_ptr<all::quick3d::Quick3DRenderer> renderer;
        Scene(all::StereoPath path)
        {
            window.resize(1544, 1080);
            renderer = std::make_unique<all::quick3d::Quick3DRenderer>(&window, camera, [](std::string_view, std::any) {});
            renderer->setStereoPath(path);
            camera.position = glm::vec3(0.0f, -0.64f, -22.9637f);
            camera.forwardVector = glm::vec3(0.0f, 0.0f, 1.0f);
            camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);
            camera.fov = 40.0f;
            camera.aspectRatio = 1544.0f / 1080.0f;
            camera.nearPlane = 0.01f;
            camera.farPlane = 54.0f;
            camera.interocularDistance = 0.5f;
            camera.convergencePlaneDistance = 25.9637f;
            renderer->createAspects(std::make_shared<all::ModelNavParameters>());
            renderer->completeInitialization();
        }
    };

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_settingsDir.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("stereo3ddemo-tests"));
        QCoreApplication::setApplicationName(QStringLiteral("quick3d-renderer"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    }

    void cleanup()
    {
        QSettings().clear();
    }

    void savedCursorOptionsReachRenderer_data()
    {
        QTest::addColumn<int>("type");
        QTest::addColumn<int>("mode");
        for (int type = 0; type < 4; ++type)
            for (int mode = 0; mode < 3; ++mode)
                QTest::newRow(qPrintable(QStringLiteral("type-%1-mode-%2").arg(type).arg(mode))) << type << mode;
    }

    void savedCursorOptionsReachRenderer()
    {
        QFETCH(int, type);
        QFETCH(int, mode);
        QSettings().setValue(QStringLiteral("cursor/type"), type);
        QSettings().setValue(QStringLiteral("cursor/displayMode"), mode);
        all::qt::MainWindow mainWindow(QSize(1544, 1080));
        auto* view = new QQuickWindow;
        mainWindow.setEmbeddedWindow(view);
        all::qt::RendererInitializer<QQuickWindow, all::quick3d::Quick3DRenderer> initializer(&mainWindow, view);
        QQuickItem* scene = nullptr;
        for (QQuickItem* child : view->contentItem()->childItems())
            if (child->objectName() == QStringLiteral("stereoScene"))
                scene = child;
        QVERIFY(scene);
        QCOMPARE(scene->property("cursorType").toInt(), type);
        QCOMPARE(scene->property("cursorVisible").toBool(), mode != int(all::CursorDisplayMode::SystemCursorOnly));
    }

    void cursorKeepsDisplayedSizeAtReducedResolution()
    {
        Scene scene(all::StereoPath::QuadBuffer);
        scene.window.setProperty("_q_stereo_view_size", QSize(1544, 1080));
        const float original = scene.renderer->sceneItem()->property("cursorScale").toFloat();
        QVERIFY(original > 0.0f);
        scene.window.resize(1029, 720);
        QCOMPARE(scene.renderer->displaySize(), QSize(1544, 1080));
        QCOMPARE(scene.renderer->sceneItem()->property("cursorScale").toFloat(), original);
        scene.window.setProperty("_q_stereo_view_size", QSize(772, 540));
        const float smallerViewScale = scene.renderer->sceneItem()->property("cursorScale").toFloat();
        QVERIFY(smallerViewScale > original * 1.99f && smallerViewScale < original * 2.01f);
    }

    void eyeCamerasAreTheSharedOffAxisFrusta()
    {
        Scene scene(all::StereoPath::QuadBuffer);
        QVERIFY(scene.renderer->sceneItem());
        QVERIFY(scene.renderer->eyeCamera(all::Eye::Left));
        QVERIFY(scene.renderer->eyeCamera(all::Eye::Right));
        const all::StereoFrustum frusta = all::stereoFrustum(all::StereoFrustumParameters{ 40.0f, 1544.0f / 1080.0f, 0.01f, 54.0f, 0.5f, 25.9637f });
        const QMatrix4x4 left = fromGlm(glm::frustum(frusta.left.left, frusta.left.right, frusta.left.bottom, frusta.left.top, 0.01f, 54.0f));
        const QMatrix4x4 right = fromGlm(glm::frustum(frusta.right.left, frusta.right.right, frusta.right.bottom, frusta.right.top, 0.01f, 54.0f));
        QVERIFY2(close(scene.renderer->eyeProjection(all::Eye::Left), left), "the left camera's projection is the shared left frustum");
        QVERIFY2(close(scene.renderer->eyeProjection(all::Eye::Right), right), "the right camera's projection is the shared right frustum");
        // The eyes sit e/2 to each side of the camera along its right vector (right = forward x up = -X for a
        // camera looking down +Z with Y up, as the original demonstrator places them)
        QCOMPARE(scene.renderer->eyePosition(all::Eye::Left).x(), 0.25f);
        QCOMPARE(scene.renderer->eyePosition(all::Eye::Right).x(), -0.25f);
        QCOMPARE(scene.renderer->eyePosition(all::Eye::Left).z(), -22.9637f);
        // What the cameras carry is what was placed
        QCOMPARE(scene.renderer->eyeCamera(all::Eye::Left)->position(), scene.renderer->eyePosition(all::Eye::Left));
    }

    void flippedEyesSwapSides()
    {
        Scene scene(all::StereoPath::QuadBuffer);
        scene.camera.flipped = true;
        scene.renderer->viewChanged();
        QCOMPARE(scene.renderer->eyePosition(all::Eye::Left).x(), -0.25f);
        QCOMPARE(scene.renderer->eyePosition(all::Eye::Right).x(), 0.25f);
    }

    void theViewFollowsTheEyeBeingDrawn()
    {
        // The quad-buffer path: one view; the edition's Qt Quick sets _q_stereo_eye (1 left, 2 right) before each pass
        Scene scene(all::StereoPath::QuadBuffer);
        QCOMPARE(scene.renderer->viewCamera(0), scene.renderer->eyeCamera(all::Eye::Left));
        scene.window.setProperty("_q_stereo_eye", 2);
        QCOMPARE(scene.renderer->currentEye(), 1);
        QCOMPARE(scene.renderer->viewCamera(0), scene.renderer->eyeCamera(all::Eye::Right));
        scene.window.setProperty("_q_stereo_eye", 1);
        QCOMPARE(scene.renderer->viewCamera(0), scene.renderer->eyeCamera(all::Eye::Left));
        // None (between frames) keeps the last eye
        scene.window.setProperty("_q_stereo_eye", 0);
        QCOMPARE(scene.renderer->viewCamera(0), scene.renderer->eyeCamera(all::Eye::Left));
        // The window asks the edition's Qt Quick to draw it once per eye
        QVERIFY(scene.window.property("_q_stereo_depth").toBool());
    }

    void thePairHoldsOneViewPerEye()
    {
        Scene scene(all::StereoPath::SideBySide);
        QCOMPARE(scene.renderer->eyeSize(), QSize(772, 1080));
        QCOMPARE(scene.renderer->viewCamera(0), scene.renderer->eyeCamera(all::Eye::Left));
        QCOMPARE(scene.renderer->viewCamera(1), scene.renderer->eyeCamera(all::Eye::Right));
        QVERIFY(!scene.window.property("_q_stereo_depth").toBool());
        // Display modes: mono shows the centre camera in both views, left and right one eye in both
        scene.renderer->propertyChanged("display_mode", all::DisplayMode::Mono);
        QCOMPARE(scene.renderer->viewCamera(0), scene.renderer->centerCamera());
        QCOMPARE(scene.renderer->viewCamera(1), scene.renderer->centerCamera());
        scene.renderer->propertyChanged("display_mode", all::DisplayMode::Right);
        QCOMPARE(scene.renderer->viewCamera(0), scene.renderer->eyeCamera(all::Eye::Right));
        scene.renderer->propertyChanged("display_mode", all::DisplayMode::Stereo);
        QCOMPARE(scene.renderer->viewCamera(0), scene.renderer->eyeCamera(all::Eye::Left));
    }

    void theRenderResolutionIsTheWindowsSize()
    {
        // The window's size is the render size; the view is shown at the size the window says
        Scene scene(all::StereoPath::QuadBuffer);
        scene.window.setProperty("_q_stereo_view_size", QSize(1544, 1080));
        scene.window.resize(1280, 720);
        QCOMPARE(scene.renderer->eyeSize(), QSize(1280, 720));
        QCOMPARE(scene.renderer->displaySize(), QSize(1544, 1080));
    }
};

QTEST_MAIN(Quick3DRendererTest)
#include "quick3d_renderer.moc"
