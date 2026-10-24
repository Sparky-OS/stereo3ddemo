#pragma once
#include <shared/cursor.h>
#include <shared/stereo_camera.h>
#include <shared/stereo_layout.h>
#include <shared/stereo_path.h>

#include <QColor>
#include <QMatrix4x4>
#include <QObject>
#include <QPointer>
#include <QQuaternion>
#include <QSize>
#include <QVector3D>

#include <glm/glm.hpp>

#include <any>
#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

class QQuickWindow;
class QQuickItem;
class QQuick3DNode;
class QQuick3DCamera;
class QQuick3DCustomCamera;
class QQuick3DViewport;
class QMouseEvent;
class QEvent;

namespace all::quick3d {

class FocusArea;
class FrustumGeometry;

// The phase 2 renderer: the scene through Qt Quick 3D in the QQuickWindow that is the 3D window. One world, three
// cameras from the shared stereo camera: the centre one picks and autofocuses, the left and right ones draw. On the
// quad-buffer path the window has a stereo surface and _q_stereo_depth, and the edition's Qt Quick draws it once per
// eye, telling the eye through _q_stereo_eye; the renderer gives the view that eye's camera. On the declared pair
// path the window is twice as wide and holds one view per eye. Each eye is drawn at the render resolution per eye:
// the window's size is that (one view, or the two side by side) and the desktop shows it at the view's size.
class Quick3DRenderer : public QObject
{
    Q_OBJECT
public:
    explicit Quick3DRenderer(QQuickWindow* view,
                             all::StereoCamera& stereoCamera,
                             std::function<void(std::string_view, std::any)> propertyUpdateNotifier);
    ~Quick3DRenderer() override;

    void viewChanged();
    void projectionChanged();
    void createAspects(std::shared_ptr<all::ModelNavParameters> nav_params);
    void completeInitialization();

    void setStereoPath(all::StereoPath path) { m_stereoPath = path; }
    all::StereoPath stereoPath() const { return m_stereoPath; }
    void setInitialModel(std::filesystem::path path) { m_initialModel = std::move(path); }

    // The render resolution per eye: the size each eye is drawn at (the window's size follows it)
    QSize eyeSize() const;
    // The size one eye is shown at (the 3D area), which the pointer's positions are in
    QSize displaySize() const;

    void loadModel(std::filesystem::path path = "assets/motorbike.obj");
    void viewAll();
    void setCursorEnabled(bool enabled);
    QQuickWindow* window() { return m_view; }
    bool hoversFocusArea(int x, int y) const;
    void showImage();
    void showModel();
    void screenshot(const std::function<void(const uint8_t* data, uint32_t width, uint32_t height)>& in);
    void onMouseEvent(QMouseEvent* event);
    void propertyChanged(std::string_view name, std::any value);
    glm::vec3 cursorWorldPosition() const;
    glm::vec3 sceneCenter() const;
    glm::vec3 sceneExtent() const;
    float fieldOfView() const;
    float aspectRatio() const;

    // For the tests: the cameras as placed, and the eye the view shows
    QQuick3DCamera* eyeCamera(all::Eye eye) const;
    QQuick3DCamera* centerCamera() const;
    QQuick3DCamera* viewCamera(int view) const;
    QMatrix4x4 eyeProjection(all::Eye eye) const;
    QVector3D eyePosition(all::Eye eye) const;
    int currentEye() const;
    QQuickItem* sceneItem() const { return m_scene; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:
    void sceneLoaded();

private:
    struct Placed {
        QVector3D position;
        QQuaternion rotation;
        QMatrix4x4 projection;
        QMatrix4x4 view;
    };
    void placeCameras();
    Placed placeEye(int eye, float interocular, bool centre) const;
    void applyPlaced(QQuick3DCustomCamera* camera, const Placed& placed);
    void updateFrustumViewer();
    void updateFocusPlane();
    void updateCursorSize();
    void updateFocusArea();
    void requestAutoFocus();
    void runAutoFocus();
    void pickCursor(QPointF displayPos);
    QVector3D unprojectOnFocusPlane(QPointF renderPos) const;
    bool rayFromCenter(QPointF renderPos, QVector3D& origin, QVector3D& direction) const;
    void setCurrentEye(int eye);
    void applyEyeSize();
    QPointF toRender(QPointF display) const;

    QQuickWindow* m_view;
    all::StereoCamera* m_stereoCamera;
    std::function<void(std::string_view, std::any)> m_propertyUpdateNotifier;
    std::shared_ptr<all::ModelNavParameters> m_navParams;
    all::StereoPath m_stereoPath = all::StereoPath::QuadBuffer;
    std::filesystem::path m_initialModel;

    QQuickItem* m_scene = nullptr;
    QQuick3DNode* m_sceneRoot = nullptr;
    QQuick3DNode* m_cursorNode = nullptr;
    QQuick3DNode* m_cursorBillboard = nullptr;
    QQuick3DNode* m_focusPlane = nullptr;
    QObject* m_loader = nullptr;
    std::array<QQuick3DCustomCamera*, 2> m_eyeCameras{ nullptr, nullptr };
    QQuick3DCustomCamera* m_centerCamera = nullptr;
    QQuick3DCustomCamera* m_frustumCamera = nullptr;
    FrustumGeometry* m_leftFrustumLines = nullptr;
    FrustumGeometry* m_rightFrustumLines = nullptr;
    FrustumGeometry* m_leftFrustumPlane = nullptr;
    FrustumGeometry* m_rightFrustumPlane = nullptr;

    std::unique_ptr<FocusArea> m_focusArea;
    all::DisplayMode m_displayMode = all::DisplayMode::Stereo;
    bool m_autoFocus = false;
    bool m_focusRequested = false;
    bool m_cursorLocked = false;
    bool m_cursorEnabled = true;
    float m_cursorScaleFactor = 1.0f;
    bool m_cursorScalingEnabled = true;
    QVector3D m_cursorPosition;
    QVector3D m_sceneCenter;
    QVector3D m_sceneExtent{ 1.0f, 1.0f, 1.0f };
    Placed m_centerPlaced;
    Placed m_eyePlaced[2];
    int m_currentEye = 0;
    QPointF m_lastPointer;
    QSize m_renderSize;
    int m_boundsTries = 0;
};

} // namespace all::quick3d
