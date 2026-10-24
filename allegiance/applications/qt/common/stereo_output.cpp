#include "stereo_output.h"

#include <QEvent>
#include <QGuiApplication>
#include <QPlatformSurfaceEvent>
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>
#include <QtGui/qguiapplication_platform.h>

#ifdef ALLEGIANCE_STEREO_DECLARE
#include <stereo-declare.h>
#include <wayland-client.h>
#if QT_CONFIG(xcb)
#include <X11/Xatom.h>
#endif
#endif

namespace all::qt {

Q_LOGGING_CATEGORY(lcStereo, "stereo3ddemo.stereo")

const char* stereoPathName(all::StereoPath path)
{
    switch (path) {
    case all::StereoPath::Off:
        return "off";
    case all::StereoPath::Unavailable:
        return "unavailable";
    case all::StereoPath::QuadBuffer:
        return "quad buffer";
    case all::StereoPath::SideBySide:
        return "side by side, declared";
    }
    return "";
}

namespace {

bool isWayland()
{
    return QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
}

#ifdef ALLEGIANCE_STEREO_DECLARE
wl_surface* waylandSurface(QWindow* window)
{
    if (!window || !window->handle())
        return nullptr;
    return static_cast<wl_surface*>(QGuiApplication::platformNativeInterface()->nativeResourceForWindow(QByteArrayLiteral("surface"), window));
}

#if QT_CONFIG(xcb)
Display* x11Display()
{
    auto* x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    return x11 ? x11->display() : nullptr;
}

// A 2D window lists its stereo children in _KDE_NET_WM_STEREO_CONTENT_CHILDREN (WINDOW/32), and each child
// declares itself; the compositor announces support for that from version 5 on.
void setStereoChildren(Display* display, Window topLevel, const std::vector<Window>& children)
{
    const Atom atom = XInternAtom(display, "_KDE_NET_WM_STEREO_CONTENT_CHILDREN", False);
    if (children.empty()) {
        XDeleteProperty(display, topLevel, atom);
    } else {
        std::vector<unsigned long> values(children.begin(), children.end());
        XChangeProperty(display, topLevel, atom, XA_WINDOW, 32, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(values.data()), int(values.size()));
    }
    XFlush(display);
}
#endif
#endif

} // namespace

void initStereoDeclaration()
{
#ifdef ALLEGIANCE_STEREO_DECLARE
    if (!isWayland())
        return;
    auto* wayland = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (!wayland || !wayland->display())
        return;
    const int r = stereo_declare_wayland_init(wayland->display(), nullptr);
    // The registry answers asynchronously; one round trip brings the compositor's globals in.
    wl_display_roundtrip(wayland->display());
    qCDebug(lcStereo) << "declaration library" << Qt::hex << stereo_declare_version() << Qt::dec << "init" << r << "supported" << stereo_supported_wayland();
#endif
}

bool stereoDeclarationSupported()
{
#ifdef ALLEGIANCE_STEREO_DECLARE
    if (isWayland())
        return stereo_supported_wayland() != 0;
#if QT_CONFIG(xcb)
    if (Display* display = x11Display())
        return stereo_supported_x11(display) >= 5;
#endif
#endif
    return false;
}

StereoDeclaration::StereoDeclaration(QWindow* window, QWindow* topLevel)
    : QObject(window)
    , m_window(window)
    , m_topLevel(topLevel)
{
    m_window->installEventFilter(this);
    declare();
}

StereoDeclaration::~StereoDeclaration()
{
    remove();
}

bool StereoDeclaration::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_window && event->type() == QEvent::PlatformSurface) {
        const auto type = static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType();
        if (type == QPlatformSurfaceEvent::SurfaceCreated)
            declare();
        else if (type == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            remove();
    }
    return QObject::eventFilter(watched, event);
}

void StereoDeclaration::declare()
{
#ifdef ALLEGIANCE_STEREO_DECLARE
    if (!m_window->handle())
        return;
    if (isWayland()) {
        wl_surface* surface = waylandSurface(m_window);
        if (!surface)
            return;
        const int r = stereo_declare_wayland(surface, STEREO_SBS_FULL, STEREO_CLASS_SCIENTIFIC, STEREO_SUBCLASS_UNSPECIFIED);
        m_declared = r == 0;
        qCDebug(lcStereo) << "declared wl_surface" << surface << "full side by side:" << r;
        return;
    }
#if QT_CONFIG(xcb)
    Display* display = x11Display();
    if (!display)
        return;
    const Window child = Window(m_window->winId());
    if (m_topLevel && m_topLevel->handle())
        setStereoChildren(display, Window(m_topLevel->winId()), { child });
    const int r = stereo_declare_x11(display, child, STEREO_SBS_FULL, STEREO_CLASS_SCIENTIFIC, STEREO_SUBCLASS_UNSPECIFIED);
    m_declared = r == 0;
    qCDebug(lcStereo) << "declared X11 window" << Qt::hex << child << Qt::dec << "full side by side:" << r;
#endif
#endif
}

void StereoDeclaration::remove()
{
#ifdef ALLEGIANCE_STEREO_DECLARE
    if (!m_declared)
        return;
    m_declared = false;
    if (isWayland()) {
        if (wl_surface* surface = waylandSurface(m_window))
            stereo_remove_wayland(surface);
        return;
    }
#if QT_CONFIG(xcb)
    if (Display* display = x11Display()) {
        if (m_window->handle())
            stereo_remove_x11(display, Window(m_window->winId()));
        if (m_topLevel && m_topLevel->handle())
            setStereoChildren(display, Window(m_topLevel->winId()), {});
    }
#endif
#endif
}

} // namespace all::qt
