#include <algorithm>
#include <applications/qt/common/main_window.h>
#include <applications/qt/common/side_menu.h>

#include <QEvent>
#include <QTimer>

namespace all::qt {

MainWindow::MainWindow(QSize size)
{
    resize(size);
    createDockWidget();
}

void MainWindow::setEmbeddedWindow(QWindow* embeddedWindow)
{
    m_embeddedWindow = embeddedWindow;
    m_embeddedContainer = QWidget::createWindowContainer(m_embeddedWindow); // Takes ownership of embeddedWindow
    m_embeddedContainer->installEventFilter(this);
    // The container sizes its window to its own rectangle whenever an ancestor moves or resizes; with two views
    // the window is resized back to hold both, right after
    connect(m_embeddedWindow, &QWindow::widthChanged, this, [this]() {
        if (m_embeddedViews > 1 || m_embeddedRenderHeight > 0)
            QTimer::singleShot(0, this, &MainWindow::applyEmbeddedGeometry);
    });
    setCentralWidget(m_embeddedContainer);
}

void MainWindow::setEmbeddedViews(int views)
{
    m_embeddedViews = views;
    // The platform shows a window that holds its views side by side at one view's width (Sparky Stereo OS's
    // Qt, _q_stereo_side_by_side_views); read when the window's geometry changes, which follows
    if (m_embeddedWindow)
        m_embeddedWindow->setProperty("_q_stereo_side_by_side_views", views > 1 ? QVariant(views) : QVariant());
    applyEmbeddedGeometry();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // The container sizes its window to its own rectangle on these events; with two views the window is
    // resized afterwards to hold both
    if (watched == m_embeddedContainer && m_embeddedViews > 1) {
        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Move:
        case QEvent::Show:
            QTimer::singleShot(0, this, &MainWindow::applyEmbeddedGeometry);
            break;
        default:
            break;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setEmbeddedRenderHeight(int height)
{
    const int render = std::max(0, height);
    if (m_embeddedRenderHeight == render)
        return;
    m_embeddedRenderHeight = render;
    applyEmbeddedGeometry();
}

void MainWindow::applyEmbeddedGeometry()
{
    if (!m_embeddedWindow || !m_embeddedContainer)
        return;
    const QSize shown = m_embeddedContainer->size();
    if (m_embeddedViews <= 1 && m_embeddedRenderHeight <= 0) {
        // The window follows its container, as any embedded window; it says no view size of its own
        if (m_embeddedWindow->property("_q_stereo_view_size").isValid()) {
            m_embeddedWindow->setProperty("_q_stereo_view_size", QVariant());
            m_embeddedWindow->resize(shown);
        }
        return;
    }
    // Each eye at the render resolution (the container's size when none is chosen), the views side by side when
    // the program draws the pair; the desktop shows the window at one view's size
    const QSize eye = StereoController::renderSizeFor(m_embeddedRenderHeight, shown);
    const QRect geometry(m_embeddedContainer->mapTo(m_embeddedContainer->window(), QPoint()), QSize(eye.width() * m_embeddedViews, eye.height()));
    m_embeddedWindow->setProperty("_q_stereo_view_size", m_embeddedRenderHeight > 0 ? QVariant(shown) : QVariant());
    if (m_embeddedWindow->geometry() != geometry)
        m_embeddedWindow->setGeometry(geometry);
}

MainWindow::~MainWindow()
{
}

QWindow* MainWindow::embeddedWindow() const noexcept
{
    return m_embeddedWindow;
}

SideMenu* MainWindow::sideMenu()
{
    return m_sideMenu;
}

// Issue: unstable focus on camera control widget resizing
bool MainWindow::onKeyPress(QKeyEvent* e)
{
    switch (e->key()) {
    case Qt::Key_F11:
        setWindowState(isFullScreen() ? Qt::WindowNoState : Qt::WindowFullScreen);
        return true;
    case Qt::Key_Return:
        if (e->modifiers() & Qt::AltModifier)
            setWindowState(isFullScreen() ? Qt::WindowNoState : Qt::WindowFullScreen);
        return true;
    case Qt::Key_Escape:
        if (isFullScreen())
            setWindowState(Qt::WindowNoState);
        return true;
    case Qt::Key_Space:
        m_sideMenu->cursorController()->cycleDisplayMode();
        return true;
    case Qt::Key_F12:
        Q_EMIT onScreenshot();
        return true;
    default:
        break;
    }
    if (m_sideMenu != nullptr)
        return m_sideMenu->onKeyPress(e);
    return false;
}

bool MainWindow::onKeyRelease(QKeyEvent* e)
{
    if (m_sideMenu != nullptr)
        return m_sideMenu->onKeyRelease(e);
    return false;
}

bool MainWindow::isShiftPressed() const
{
    return m_sideMenu->isShiftPressed();
}

void MainWindow::setMouseGlobalPosition(int x, int y)
{
    m_mouseGlobalPositionX = x;
    m_mouseGlobalPositionY = y;

    m_sideMenu->sceneController()->setMousePressedX(static_cast<float>(x));
}

QPoint MainWindow::mouseGlobalPosition() const
{
    return { m_mouseGlobalPositionX, m_mouseGlobalPositionY };
}

void MainWindow::setMousePressed(bool pressed)
{
    m_mousePressed = pressed;
}

bool MainWindow::mousePressed() const
{
    return m_mousePressed;
}

void MainWindow::mouseHoverOveringOver3DView()
{
    const bool systemCursor = m_sideMenu->cursorController()->displayMode() == CursorController::CursorDisplayMode::Both ||
            m_sideMenu->cursorController()->displayMode() == CursorController::CursorDisplayMode::SystemCursorOnly;
    const QCursor cursor(systemCursor ? Qt::ArrowCursor : Qt::BlankCursor);
    setCursor(cursor);
    // The 3D window is a window of its own: the pointer over it takes that window's cursor
    if (m_embeddedWindow)
        m_embeddedWindow->setCursor(cursor);
}

bool MainWindow::lockMouseInPlace() const
{
    return m_sideMenu->sceneController()->lockMouseInPlace();
}

void MainWindow::createDockWidget()
{
    QDockWidget* dock = new QDockWidget("Camera", this);
    m_sideMenu = new SideMenu(dock);
    dock->setWidget(m_sideMenu);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    connect(dock, &QDockWidget::dockLocationChanged, m_sideMenu, &SideMenu::setClearColor);
}

} // namespace all::qt
