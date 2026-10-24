#pragma once
#include <QMainWindow>
#include <QMenuBar>
#include <QDockWidget>
#include <QKeyEvent>

namespace all::qt {

class SideMenu;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QSize size);
    ~MainWindow();

    QWindow* embeddedWindow() const noexcept;
    void setEmbeddedWindow(QWindow* embeddedWindow); // Takes ownership of embeddedWindow;

    // How many views the embedded window holds side by side: 2 makes it twice as wide as its place in the
    // window, so each eye keeps the size of the 3D area (the desktop shows it at one view's size).
    void setEmbeddedViews(int views);
    int embeddedViews() const noexcept { return m_embeddedViews; }

    SideMenu* sideMenu();

    bool onKeyPress(QKeyEvent* e);
    bool onKeyRelease(QKeyEvent* e);

    [[nodiscard]] bool isShiftPressed() const;

    void setMouseGlobalPosition(int x, int y);
    [[nodiscard]] QPoint mouseGlobalPosition() const;

    void setMousePressed(bool pressed);
    [[nodiscard]] bool mousePressed() const;

    void mouseHoverOveringOver3DView();

    // This is controlled from within QML to request that mouse is to be "locked in place".
    // Used for the slider incremental adjustments.
    [[nodiscard]] bool lockMouseInPlace() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void createDockWidget();
    void applyEmbeddedGeometry();

Q_SIGNALS:
    void onClose();
    void onScreenshot();

private:
    QWindow* m_embeddedWindow{ nullptr };
    QWidget* m_embeddedContainer{ nullptr };
    int m_embeddedViews{ 1 };
    SideMenu* m_sideMenu;
    bool m_cursor = true;

    bool m_mousePressed = false;
    int m_mouseGlobalPositionX{ 0 };
    int m_mouseGlobalPositionY{ 0 };
};
} // namespace all::qt
