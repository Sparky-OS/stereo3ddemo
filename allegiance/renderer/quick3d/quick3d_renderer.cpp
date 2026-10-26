#include "quick3d_renderer.h"

#include "focus_area.h"
#include "frustum_geometry.h"

#include <shared/stereo_frustum.h>

// Qt Quick 3D's private headers spell the Qt keywords, which this build turns off (KDBindings names emit)
#define signals public
#define slots
#include <QtQuick3D/private/qquick3dcustomcamera_p.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dpickresult_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#undef signals
#undef slots

#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFileInfo>
#include <QVector4D>
#include <QImage>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>
#include <QtMath>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>

Q_LOGGING_CATEGORY(lcQuick3D, "stereo3ddemo.quick3d")

namespace all::quick3d {

namespace {

constexpr int AFSamplesX = 2;
constexpr int AFSamplesY = 2;

QVector3D toQVector3D(const glm::vec3& v)
{
    return QVector3D(v.x, v.y, v.z);
}

glm::vec3 toGlmVec3(const QVector3D& v)
{
    return glm::vec3(v.x(), v.y(), v.z());
}

QMatrix4x4 toQMatrix4x4(const glm::mat4& m)
{
    // glm is column major; QMatrix4x4's float constructor takes rows
    QMatrix4x4 q;
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            q(r, c) = m[c][r];
    return q;
}

// An object by name, through the QObject children and the items' visual children (a Repeater's delegates are
// visual children of its parent item, not QObject children of the scene)
template<typename T>
T* findObject(QObject* root, const char* name)
{
    if (!root)
        return nullptr;
    if (root->objectName() == QLatin1String(name))
        if (auto* t = qobject_cast<T*>(root))
            return t;
    if (auto* t = root->findChild<T*>(QString::fromLatin1(name)))
        return t;
    if (auto* item = qobject_cast<QQuickItem*>(root)) {
        const QList<QQuickItem*> children = item->childItems();
        for (QQuickItem* child : children)
            if (auto* t = findObject<T>(child, name))
                return t;
    }
    return nullptr;
}

} // namespace

Quick3DRenderer::Quick3DRenderer(QQuickWindow* view,
                                 all::StereoCamera& stereoCamera,
                                 std::function<void(std::string_view, std::any)> propertyUpdateNotifier)
    : m_view(view)
    , m_stereoCamera(&stereoCamera)
    , m_propertyUpdateNotifier(std::move(propertyUpdateNotifier))
    , m_focusArea(std::make_unique<FocusArea>())
{
    QObject::connect(m_focusArea.get(), &FocusArea::changed, this, [this] {
        updateFocusArea();
        requestAutoFocus();
    });
    QObject::connect(m_focusArea.get(), &FocusArea::hoverChanged, this, &Quick3DRenderer::updateFocusArea);
}

Quick3DRenderer::~Quick3DRenderer()
{
    if (m_view)
        m_view->removeEventFilter(this);
    delete m_scene;
}

void Quick3DRenderer::createAspects(std::shared_ptr<all::ModelNavParameters> nav_params)
{
    m_navParams = std::move(nav_params);

    // The quad-buffer path: the edition's Qt Quick draws the window once per eye and says which
    if (m_stereoPath == all::StereoPath::QuadBuffer)
        m_view->setProperty("_q_stereo_depth", true);
    m_view->installEventFilter(this);

    auto* engine = new QQmlEngine(this);
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/qt/qml/Stereo3D/Quick3D/qml/StereoScene.qml")));
    if (component.isError()) {
        for (const QQmlError& error : component.errors())
            qCWarning(lcQuick3D) << error.toString();
        return;
    }
    QObject* object = component.create();
    m_scene = qobject_cast<QQuickItem*>(object);
    if (!m_scene) {
        qCWarning(lcQuick3D) << "the scene did not load";
        delete object;
        return;
    }
    m_scene->setParentItem(m_view->contentItem());
    m_scene->setParent(this);

    m_sceneRoot = findObject<QQuick3DNode>(m_scene, "sceneRoot");
    m_eyeCameras[0] = findObject<QQuick3DCustomCamera>(m_scene, "leftCamera");
    m_eyeCameras[1] = findObject<QQuick3DCustomCamera>(m_scene, "rightCamera");
    m_centerCamera = findObject<QQuick3DCustomCamera>(m_scene, "centerCamera");
    m_frustumCamera = findObject<QQuick3DCustomCamera>(m_scene, "frustumCamera");
    m_cursorNode = findObject<QQuick3DNode>(m_scene, "cursorNode");
    m_cursorBillboard = findObject<QQuick3DNode>(m_scene, "cursorBillboard");
    m_focusPlane = findObject<QQuick3DNode>(m_scene, "focusPlane");
    m_loader = findObject<QObject>(m_scene, "loader");
    m_leftFrustumLines = findObject<FrustumGeometry>(m_scene, "leftFrustumLines");
    m_rightFrustumLines = findObject<FrustumGeometry>(m_scene, "rightFrustumLines");
    m_leftFrustumPlane = findObject<FrustumGeometry>(m_scene, "leftFrustumPlane");
    m_rightFrustumPlane = findObject<FrustumGeometry>(m_scene, "rightFrustumPlane");

    // The original demonstrator's four lights: a light shines along its -Z
    const std::array<QVector3D, 4> lightDirs{ QVector3D(1, -1, 1), QVector3D(-1, -1, -1), QVector3D(1, 1, -1), QVector3D(-1, 1, 1) };
    for (size_t i = 0; i < lightDirs.size(); ++i) {
        if (auto* light = m_scene->findChild<QQuick3DNode*>(QStringLiteral("light%1").arg(i)))
            light->setRotation(QQuaternion::fromDirection(-lightDirs[i].normalized(), QVector3D(0, 1, 0)));
    }

    m_scene->setProperty("views", all::stereoViews(m_stereoPath));
    m_scene->setProperty("stereoDisabled", m_stereoPath == all::StereoPath::Off);
    m_scene->setProperty("cursorColor", QColor(Qt::white));

    if (m_loader)
        QObject::connect(m_loader, SIGNAL(statusChanged()), this, SLOT(sceneLoaded()));

    // The window's size is the render size (one view, or the two side by side)
    QObject::connect(m_view, &QWindow::widthChanged, this, &Quick3DRenderer::applyEyeSize);
    QObject::connect(m_view, &QWindow::heightChanged, this, &Quick3DRenderer::applyEyeSize);
    applyEyeSize();

    if (m_navParams) {
        m_navParams->hit_test = [this](glm::vec3 origin, glm::vec3 direction) -> glm::vec3 {
            auto* view = findObject<QQuick3DViewport>(m_scene, "leftView");
            if (!view)
                return glm::vec3{ -1 };
            const QQuick3DPickResult hit = view->rayPick(toQVector3D(origin), toQVector3D(direction));
            return hit.objectHit() ? toGlmVec3(hit.scenePosition()) : glm::vec3{ -1 };
        };
    }

    if (m_initialModel.empty())
        loadModel();
    else
        loadModel(m_initialModel);
}

void Quick3DRenderer::completeInitialization()
{
    placeCameras();
}

void Quick3DRenderer::applyEyeSize()
{
    const QSize eye = eyeSize();
    if (!m_scene)
        return;
    m_scene->setProperty("eyeWidth", eye.width());
    m_scene->setProperty("eyeHeight", eye.height());
    m_focusArea->setViewSize(QSizeF(eye));
    updateCursorSize();
}

QSize Quick3DRenderer::eyeSize() const
{
    return QSize(m_view->width() / all::stereoViews(m_stereoPath), m_view->height());
}

QSize Quick3DRenderer::displaySize() const
{
    const QSize shown = m_view->property("_q_stereo_view_size").toSize();
    return (shown.width() > 0 && shown.height() > 0) ? shown : eyeSize();
}

QPointF Quick3DRenderer::toRender(QPointF display) const
{
    const QSize d = displaySize();
    const QSize r = eyeSize();
    const all::ViewPoint p = all::displayToRender(float(display.x()), float(display.y()),
                                                  all::ViewSize{ d.width(), d.height() }, all::ViewSize{ r.width(), r.height() });
    return QPointF(p.x, p.y);
}

bool Quick3DRenderer::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_view && event->type() == QEvent::DynamicPropertyChange) {
        auto* change = static_cast<QDynamicPropertyChangeEvent*>(event);
        if (change->propertyName() == "_q_stereo_eye") {
            // 0 none, 1 left, 2 right (the edition's Qt Quick); none keeps the last eye's camera
            const int eye = m_view->property("_q_stereo_eye").toInt();
            if (eye > 0)
                setCurrentEye(eye - 1);
        } else if (change->propertyName() == "_q_stereo_view_size") {
            m_focusArea->setViewSize(QSizeF(eyeSize()));
            updateCursorSize();
        }
    }
    return QObject::eventFilter(watched, event);
}

void Quick3DRenderer::setCurrentEye(int eye)
{
    if (m_currentEye == eye)
        return;
    m_currentEye = eye;
    if (m_scene)
        m_scene->setProperty("currentEye", eye);
}

int Quick3DRenderer::currentEye() const
{
    return m_currentEye;
}

Quick3DRenderer::Placed Quick3DRenderer::placeEye(int eye, float interocular, bool centre) const
{
    const glm::vec3 forward = glm::normalize(m_stereoCamera->forwardVector());
    const glm::vec3 up = glm::normalize(m_stereoCamera->upVector());
    const glm::vec3 right = glm::normalize(glm::cross(forward, up));
    const float d = m_stereoCamera->convergencePlaneDistance();
    const float n = m_stereoCamera->nearPlane();
    const float f = m_stereoCamera->farPlane();

    // The shared off-axis maths: each eye displaced along the camera's right vector, its window shifted toward
    // the other eye; a negative interocular distance swaps the eyes
    const all::StereoFrustum frusta = all::stereoFrustum(all::StereoFrustumParameters{
            m_stereoCamera->fov(), m_stereoCamera->aspectRatio(), n, f, interocular, d });
    const all::EyeFrustum& e = eye == 0 ? frusta.left : frusta.right;
    const float eyeOffset = centre ? 0.0f : e.eyeOffset;
    const glm::vec3 position = m_stereoCamera->position() + right * eyeOffset;

    Placed placed;
    placed.position = toQVector3D(position);

    // The camera looks down its -Z: for the asymmetric frustum the axes stay parallel; toe-in turns each eye
    // toward the convergence point
    glm::vec3 lookDir = forward;
    if (!centre && m_stereoCamera->mode() == all::StereoCamera::Mode::ToeIn) {
        const glm::vec3 viewCenter = m_stereoCamera->position() + forward * d;
        lookDir = glm::normalize(viewCenter - position);
    }
    const glm::vec3 r = glm::normalize(glm::cross(lookDir, up));
    const glm::vec3 u = glm::cross(r, lookDir);
    placed.rotation = QQuaternion::fromAxes(toQVector3D(r), toQVector3D(u), toQVector3D(-lookDir));

    if (centre || m_stereoCamera->mode() == all::StereoCamera::Mode::ToeIn)
        placed.projection = toQMatrix4x4(glm::perspective(glm::radians(m_stereoCamera->fov()), m_stereoCamera->aspectRatio(), n, f));
    else
        placed.projection = toQMatrix4x4(glm::frustum(e.left, e.right, e.bottom, e.top, e.nearPlane, e.farPlane));
    QMatrix4x4 view;
    view.rotate(placed.rotation.conjugated());
    view.translate(-placed.position);
    placed.view = view;
    return placed;
}

void Quick3DRenderer::applyPlaced(QQuick3DCustomCamera* camera, const Placed& placed)
{
    if (!camera)
        return;
    camera->setPosition(placed.position);
    camera->setRotation(placed.rotation);
    camera->setProjection(placed.projection);
}

void Quick3DRenderer::placeCameras()
{
    const float flippedCorrection = m_stereoCamera->flipped() ? -1.0f : 1.0f;
    const float interocular = flippedCorrection * m_stereoCamera->interocularDistance();
    m_eyePlaced[0] = placeEye(0, interocular, false);
    m_eyePlaced[1] = placeEye(1, interocular, false);
    m_centerPlaced = placeEye(0, interocular, true);
    applyPlaced(m_eyeCameras[0], m_eyePlaced[0]);
    applyPlaced(m_eyeCameras[1], m_eyePlaced[1]);
    applyPlaced(m_centerCamera, m_centerPlaced);
    qCDebug(lcQuick3D).nospace() << "camera position " << toQVector3D(m_stereoCamera->position()) << " forward " << toQVector3D(m_stereoCamera->forwardVector())
                                 << " up " << toQVector3D(m_stereoCamera->upVector()) << " fov " << m_stereoCamera->fov() << " aspect " << m_stereoCamera->aspectRatio()
                                 << " near " << m_stereoCamera->nearPlane() << " far " << m_stereoCamera->farPlane() << " e " << interocular
                                 << " d " << m_stereoCamera->convergencePlaneDistance() << " mode " << int(m_stereoCamera->mode())
                                 << " eye " << eyeSize() << " window " << m_view->size() << " shown " << displaySize();
    updateFrustumViewer();
    updateFocusPlane();
    updateCursorSize();
    requestAutoFocus();
}

void Quick3DRenderer::viewChanged()
{
    placeCameras();
}

void Quick3DRenderer::projectionChanged()
{
    placeCameras();
}

void Quick3DRenderer::updateFrustumViewer()
{
    if (!m_leftFrustumLines || !m_frustumCamera)
        return;
    // The frusta drawn with the eyes twenty times apart, seen from above
    const float flippedCorrection = m_stereoCamera->flipped() ? -1.0f : 1.0f;
    const float interocular = flippedCorrection * m_stereoCamera->interocularDistance() * 20.0f;
    const Placed amplified[2] = { placeEye(0, interocular, false), placeEye(1, interocular, false) };
    const float d = m_stereoCamera->convergencePlaneDistance();
    const float n = m_stereoCamera->nearPlane();
    const float f = m_stereoCamera->farPlane();
    FrustumGeometry* lines[2] = { m_leftFrustumLines, m_rightFrustumLines };
    FrustumGeometry* planes[2] = { m_leftFrustumPlane, m_rightFrustumPlane };
    for (int eye = 0; eye < 2; ++eye) {
        const Placed& p = amplified[eye];
        // The convergence plane's depth in clip space
        const QVector4D zc = p.projection * QVector4D(0.0f, 0.0f, -d, 1.0f);
        const float zFocus = zc.z() / zc.w();
        const QMatrix4x4 inverse = (p.projection * p.view).inverted();
        auto unproject = [&](float x, float y, float z) {
            QVector4D v = inverse * QVector4D(x, y, z, 1.0f);
            return v.toVector3DAffine();
        };
        const QVector3D nl = unproject(-1, 0, -1), nr = unproject(1, 0, -1);
        const QVector3D fl = unproject(-1, 0, 1), fr = unproject(1, 0, 1);
        const QVector3D cl = unproject(-1, 0, zFocus), cr = unproject(1, 0, zFocus);
        const QVector3D ct = unproject(0, 1, zFocus), cb = unproject(0, -1, zFocus);
        Q_UNUSED(n);
        Q_UNUSED(f);
        if (lines[eye])
            lines[eye]->setVertices({ nl, fl, nr, fr, nl, nr, fl, fr, cl, cr });
        if (planes[eye])
            planes[eye]->setVertices({ cl, ct, cr, cl, cr, cb });
    }
    // An orthographic view from above the centre camera, wide enough for the whole frustum
    const float frustumLength = f - n;
    const float vFov = qDegreesToRadians(m_stereoCamera->fov());
    const float hFov = 2.0f * std::atan(m_stereoCamera->aspectRatio() * std::tan(vFov * 0.5f));
    const float shift = m_stereoCamera->mode() == all::StereoCamera::Mode::AsymmetricFrustum ? 2.0f * std::fabs(interocular) : 0.0f;
    const float halfWidth = frustumLength * std::tan(hFov * 0.5f) + shift;
    const float halfSize = std::max(halfWidth, frustumLength * 0.5f);
    const glm::vec3 forward = glm::normalize(m_stereoCamera->forwardVector());
    const glm::vec3 up = glm::normalize(m_stereoCamera->upVector());
    const glm::vec3 centre = m_stereoCamera->position() + forward * (n + frustumLength * 0.5f);
    const glm::vec3 eyePos = centre + up * (f * 1.5f);
    const glm::vec3 r = glm::normalize(glm::cross(forward, up));
    // Looking down: -Z toward -up, Y toward forward
    m_frustumCamera->setPosition(toQVector3D(eyePos));
    m_frustumCamera->setRotation(QQuaternion::fromAxes(toQVector3D(r), toQVector3D(forward), toQVector3D(up)));
    const float aspect = m_stereoCamera->aspectRatio();
    m_frustumCamera->setProjection(toQMatrix4x4(glm::ortho(-halfSize, halfSize, -halfSize * aspect, halfSize * aspect, 0.01f, f * 3.0f)));
}

void Quick3DRenderer::updateFocusPlane()
{
    if (!m_focusPlane)
        return;
    // A quad filling the centre camera's view at the convergence distance (the #Rectangle is 100 units wide)
    const float d = m_stereoCamera->convergencePlaneDistance();
    const float halfHeight = std::tan(qDegreesToRadians(m_stereoCamera->fov()) * 0.5f) * d;
    const float halfWidth = halfHeight * m_stereoCamera->aspectRatio();
    const glm::vec3 forward = glm::normalize(m_stereoCamera->forwardVector());
    m_focusPlane->setPosition(toQVector3D(m_stereoCamera->position() + forward * d));
    m_focusPlane->setRotation(m_centerPlaced.rotation);
    m_focusPlane->setScale(QVector3D(halfWidth * 2.0f / 100.0f, halfHeight * 2.0f / 100.0f, 1.0f));
}

void Quick3DRenderer::updateCursorSize()
{
    if (!m_cursorNode || !m_scene)
        return;
    // The original demonstrator's sizing: a constant 20 pixels on screen, or a constant world size, times the user's
    // scale factor
    constexpr float cursorSize = 0.06f;
    constexpr float targetPixels = 20.0f;
    const QVector3D cameraPosition = m_centerPlaced.position;
    const float distance = (m_cursorPosition - cameraPosition).length();
    const float fov = 2.0f * std::atan(1.0f / std::max(m_centerPlaced.projection(0, 0), 1e-4f));
    const int viewWidth = std::max(displaySize().width(), 1);
    const float pixelsToAngle = fov / float(viewWidth);
    const float radius = distance * std::tan(targetPixels * pixelsToAngle * 0.5f);
    const float scale = m_cursorScaleFactor * (m_cursorScalingEnabled ? radius : cursorSize);
    m_scene->setProperty("cursorScale", scale);
    m_cursorNode->setPosition(m_cursorPosition);
    if (m_cursorBillboard) {
        const QVector3D direction = (cameraPosition - m_cursorPosition).normalized();
        const QVector3D up = m_centerPlaced.rotation.rotatedVector(QVector3D(0, 1, 0));
        m_cursorBillboard->setRotation(QQuaternion::fromDirection(direction, up));
    }
}

void Quick3DRenderer::updateFocusArea()
{
    if (!m_scene)
        return;
    m_scene->setProperty("focusAreaCenterX", m_focusArea->center().x());
    m_scene->setProperty("focusAreaCenterY", m_focusArea->center().y());
    m_scene->setProperty("focusAreaWidth", m_focusArea->extent().width());
    m_scene->setProperty("focusAreaHeight", m_focusArea->extent().height());
    m_scene->setProperty("focusAreaHover", int(m_focusArea->hover()));
    m_scene->setProperty("showFocusArea", m_focusArea->enabled());
}

void Quick3DRenderer::requestAutoFocus()
{
    if (m_focusRequested || !m_autoFocus)
        return;
    m_focusRequested = true;
    QMetaObject::invokeMethod(this, &Quick3DRenderer::runAutoFocus, Qt::QueuedConnection);
}

bool Quick3DRenderer::rayFromCenter(QPointF renderPos, QVector3D& origin, QVector3D& direction) const
{
    const QSize eye = eyeSize();
    if (eye.width() <= 0 || eye.height() <= 0)
        return false;
    const QMatrix4x4 inverse = (m_centerPlaced.projection * m_centerPlaced.view).inverted();
    const float nx = float(renderPos.x()) / float(eye.width()) * 2.0f - 1.0f;
    const float ny = 1.0f - float(renderPos.y()) / float(eye.height()) * 2.0f;
    const QVector3D nearPoint = (inverse * QVector4D(nx, ny, -1.0f, 1.0f)).toVector3DAffine();
    const QVector3D farPoint = (inverse * QVector4D(nx, ny, 1.0f, 1.0f)).toVector3DAffine();
    origin = m_centerPlaced.position;
    direction = (farPoint - nearPoint).normalized();
    return true;
}

void Quick3DRenderer::runAutoFocus()
{
    m_focusRequested = false;
    auto* view = findObject<QQuick3DViewport>(m_scene, "leftView");
    if (!view || !m_autoFocus)
        return;
    // The original demonstrator's 2 x 2 rays over the focus area, the nearest hit of each, averaged
    const QPointF center = m_focusArea->center();
    const QSizeF extent = m_focusArea->extent();
    float sum = 0.0f;
    int valid = 0;
    for (int y = 0; y < AFSamplesY; ++y) {
        for (int x = 0; x < AFSamplesX; ++x) {
            const QPointF p(center.x() - extent.width() * 0.5 + x * extent.width() / (AFSamplesX - 1),
                            center.y() - extent.height() * 0.5 + y * extent.height() / (AFSamplesY - 1));
            QVector3D origin, direction;
            if (!rayFromCenter(p, origin, direction))
                continue;
            const QQuick3DPickResult hit = view->rayPick(origin, direction);
            if (hit.objectHit()) {
                sum += (hit.scenePosition() - origin).length();
                ++valid;
            }
        }
    }
    if (valid > 0)
        m_propertyUpdateNotifier("auto_focus_distance", sum / float(valid));
}

QVector3D Quick3DRenderer::unprojectOnFocusPlane(QPointF renderPos) const
{
    const float d = m_stereoCamera->convergencePlaneDistance();
    const glm::vec3 forward = glm::normalize(m_stereoCamera->forwardVector());
    const QVector3D viewCenter = toQVector3D(m_stereoCamera->position() + forward * d);
    const QVector4D clip = m_centerPlaced.projection * m_centerPlaced.view * QVector4D(viewCenter, 1.0f);
    const float zFocus = clip.z() / clip.w();
    const QSize eye = eyeSize();
    const float nx = float(renderPos.x()) / float(std::max(eye.width(), 1)) * 2.0f - 1.0f;
    const float ny = 1.0f - float(renderPos.y()) / float(std::max(eye.height(), 1)) * 2.0f;
    const QMatrix4x4 inverse = (m_centerPlaced.projection * m_centerPlaced.view).inverted();
    return (inverse * QVector4D(nx, ny, zFocus, 1.0f)).toVector3DAffine();
}

void Quick3DRenderer::pickCursor(QPointF displayPos)
{
    auto* view = findObject<QQuick3DViewport>(m_scene, "leftView");
    const QPointF renderPos = toRender(displayPos);
    QVector3D origin, direction;
    if (!view || !rayFromCenter(renderPos, origin, direction))
        return;
    const QQuick3DPickResult hit = view->rayPick(origin, direction);
    if (hit.objectHit()) {
        qCDebug(lcQuick3D) << "cursor hit" << hit.objectHit()->objectName() << "at" << hit.scenePosition() << "distance" << hit.distance();
        m_cursorPosition = hit.scenePosition();
    } else {
        qCDebug(lcQuick3D) << "cursor at" << renderPos << "hits nothing: on the focus plane at" << m_stereoCamera->convergencePlaneDistance();
        m_cursorPosition = unprojectOnFocusPlane(renderPos);
    }
    updateCursorSize();
}

void Quick3DRenderer::onMouseEvent(QMouseEvent* event)
{
    const QPointF renderPos = toRender(event->position());
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        m_focusArea->mousePressed(renderPos);
        break;
    case QEvent::MouseButtonRelease:
        m_focusArea->mouseReleased();
        break;
    case QEvent::MouseMove:
        m_focusArea->mouseMoved(renderPos);
        m_lastPointer = event->position();
        if (!m_cursorLocked)
            pickCursor(event->position());
        break;
    default:
        break;
    }
}

void Quick3DRenderer::propertyChanged(std::string_view name, std::any value)
{
    if (!m_scene)
        return;
    if (name == "cursor_scale_factor") {
        m_cursorScaleFactor = std::any_cast<float>(value);
        updateCursorSize();
    } else if (name == "cursor_type") {
        m_scene->setProperty("cursorType", int(std::any_cast<CursorType>(value)));
    } else if (name == "display_mode") {
        m_displayMode = std::any_cast<all::DisplayMode>(value);
        m_scene->setProperty("displayMode", int(m_displayMode));
    } else if (name == "frustum_view_enabled") {
        m_scene->setProperty("showFrustum", std::any_cast<bool>(value));
    } else if (name == "show_focus_area") {
        m_focusArea->setEnabled(std::any_cast<bool>(value));
        updateFocusArea();
    } else if (name == "auto_focus") {
        m_autoFocus = std::any_cast<bool>(value);
        if (m_autoFocus)
            requestAutoFocus();
    } else if (name == "cursor_color") {
        const auto c = std::any_cast<std::array<float, 4>>(value);
        m_scene->setProperty("cursorColor", QColor::fromRgbF(c[0], c[1], c[2], c[3]));
    } else if (name == "show_focus_plane") {
        m_scene->setProperty("showFocusPlane", std::any_cast<bool>(value));
    } else if (name == "wireframe_enabled") {
        m_scene->setProperty("wireframe", std::any_cast<bool>(value));
    } else if (name == "cursor_locked") {
        m_cursorLocked = std::any_cast<bool>(value);
    }
}

glm::vec3 Quick3DRenderer::cursorWorldPosition() const
{
    return toGlmVec3(m_cursorPosition);
}

glm::vec3 Quick3DRenderer::sceneCenter() const
{
    return toGlmVec3(m_sceneCenter);
}

glm::vec3 Quick3DRenderer::sceneExtent() const
{
    return toGlmVec3(m_sceneExtent);
}

float Quick3DRenderer::fieldOfView() const
{
    return m_stereoCamera->fov();
}

float Quick3DRenderer::aspectRatio() const
{
    return m_stereoCamera->aspectRatio();
}

void Quick3DRenderer::loadModel(std::filesystem::path path)
{
    if (!m_scene)
        return;
    QString filePath = QString::fromStdString(path.string());
#ifdef ALLEGIANCE_DATADIR
    // The bundled assets live next to the program when run from its build folder, else in the data folder
    if (path.is_relative() && !std::filesystem::exists(path) && std::filesystem::exists(std::filesystem::path(ALLEGIANCE_DATADIR) / path))
        filePath = QString::fromStdString((std::filesystem::path(ALLEGIANCE_DATADIR) / path).string());
#endif
    qCDebug(lcQuick3D) << "loading" << filePath;
    m_scene->setProperty("imageMode", false);
    m_scene->setProperty("modelSource", QUrl::fromLocalFile(QFileInfo(filePath).absoluteFilePath()));
}

// The box of the loaded models, each model's bounds (set by the scene manager once its mesh is in) taken to the
// scene; empty until then
static void accumulateBounds(QObject* node, QVector3D& min, QVector3D& max, bool& any)
{
    if (auto* model = qobject_cast<QQuick3DModel*>(node)) {
        const QQuick3DBounds3 b = model->bounds();
        const QVector3D bmin = b.minimum();
        const QVector3D bmax = b.maximum();
        if (bmax.x() > bmin.x() || bmax.y() > bmin.y() || bmax.z() > bmin.z()) {
            const QMatrix4x4 transform = model->sceneTransform();
            for (int i = 0; i < 8; ++i) {
                const QVector3D corner(i & 1 ? bmax.x() : bmin.x(), i & 2 ? bmax.y() : bmin.y(), i & 4 ? bmax.z() : bmin.z());
                const QVector3D p = transform.map(corner);
                if (!any) {
                    min = max = p;
                    any = true;
                } else {
                    min = QVector3D(std::min(min.x(), p.x()), std::min(min.y(), p.y()), std::min(min.z(), p.z()));
                    max = QVector3D(std::max(max.x(), p.x()), std::max(max.y(), p.y()), std::max(max.z(), p.z()));
                }
            }
        }
    }
    const QList<QObject*> children = node->children();
    for (QObject* child : children)
        accumulateBounds(child, min, max, any);
}

static void makePickable(QObject* node)
{
    if (auto* model = qobject_cast<QQuick3DModel*>(node))
        model->setPickable(true);
    const QList<QObject*> children = node->children();
    for (QObject* child : children)
        makePickable(child);
}

void Quick3DRenderer::sceneLoaded()
{
    if (!m_loader)
        return;
    const int status = m_loader->property("status").toInt(); // RuntimeLoader.Status: 0 empty, 1 success, 2 error
    if (status == 2) {
        qCWarning(lcQuick3D) << "Failed to load model:" << m_loader->property("errorString").toString();
        return;
    }
    if (status != 1)
        return;
    // The models' bounds arrive after a first synchronization of the scene; until then the renderer asks again
    QVector3D min, max;
    bool any = false;
    accumulateBounds(m_loader, min, max, any);
    if (!any) {
        if (++m_boundsTries < 200)
            QTimer::singleShot(50, this, &Quick3DRenderer::sceneLoaded);
        return;
    }
    m_boundsTries = 0;
    // The cursor and the autofocus pick the loaded models: Qt Quick 3D picks only models marked pickable
    makePickable(m_loader);
    m_sceneCenter = (min + max) * 0.5f;
    m_sceneExtent = max - min;
    if (m_navParams) {
        m_navParams->min_extent = toGlmVec3(min);
        m_navParams->max_extent = toGlmVec3(max);
    }
    qCDebug(lcQuick3D) << "scene loaded, centre" << m_sceneCenter << "extent" << m_sceneExtent;
    m_propertyUpdateNotifier("scene_loaded", {});
}

void Quick3DRenderer::viewAll()
{
    if (m_sceneExtent.lengthSquared() > 0.0f)
        m_propertyUpdateNotifier("scene_loaded", {});
}

void Quick3DRenderer::setCursorEnabled(bool enabled)
{
    m_cursorEnabled = enabled;
    if (m_scene)
        m_scene->setProperty("cursorVisible", enabled);
}

bool Quick3DRenderer::hoversFocusArea(int, int) const
{
    return m_focusArea->containsMouse();
}

void Quick3DRenderer::showImage()
{
    if (!m_scene)
        return;
    m_scene->setProperty("imageSource", QUrl(QStringLiteral("qrc:/13_3840x2160_sbs.jpg")));
    m_scene->setProperty("imageMode", true);
}

void Quick3DRenderer::showModel()
{
    if (m_scene)
        m_scene->setProperty("imageMode", false);
}

void Quick3DRenderer::screenshot(const std::function<void(const uint8_t* data, uint32_t width, uint32_t height)>& in)
{
    const QImage image = m_view->grabWindow().convertToFormat(QImage::Format_RGBA8888);
    if (!image.isNull())
        in(image.constBits(), uint32_t(image.width()), uint32_t(image.height()));
}

QQuick3DCamera* Quick3DRenderer::eyeCamera(all::Eye eye) const
{
    return m_eyeCameras[size_t(eye)];
}

QQuick3DCamera* Quick3DRenderer::centerCamera() const
{
    return m_centerCamera;
}

QQuick3DCamera* Quick3DRenderer::viewCamera(int view) const
{
    auto* viewport = findObject<QQuick3DViewport>(m_scene, view == 0 ? "leftView" : "rightView");
    return viewport ? viewport->camera() : nullptr;
}

QMatrix4x4 Quick3DRenderer::eyeProjection(all::Eye eye) const
{
    return m_eyePlaced[size_t(eye)].projection;
}

QVector3D Quick3DRenderer::eyePosition(all::Eye eye) const
{
    return m_eyePlaced[size_t(eye)].position;
}

} // namespace all::quick3d

