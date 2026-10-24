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
        if (m_embeddedViews > 1)
            QTimer::singleShot(0, this, &MainWindow::applyEmbeddedGeometry);
    });
    setCentralWidget(m_embeddedContainer);
}

void MainWindow::setEmbeddedViews(int views)
{
    m_embeddedViews = views;
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

void MainWindow::applyEmbeddedGeometry()
{
    if (!m_embeddedWindow || !m_embeddedContainer || m_embeddedViews <= 1)
        return;
    const QSize size = m_embeddedContainer->size();
    const QRect geometry(m_embeddedContainer->mapTo(m_embeddedContainer->window(), QPoint()), QSize(size.width() * m_embeddedViews, size.height()));
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
