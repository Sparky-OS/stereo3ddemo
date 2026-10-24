#pragma once

#include <QLoggingCategory>
#include <QObject>

#include <shared/stereo_path.h>

class QWindow;

namespace all::qt {

Q_DECLARE_LOGGING_CATEGORY(lcStereo)

const char* stereoPathName(all::StereoPath path);

// Whether the desktop takes a full side by side declaration (Stereo KWin, on Wayland or X11).
bool stereoDeclarationSupported();
// Whether the saved 3D options ask for the side by side path (read before the application exists).
bool sideBySideChosen();
// Whether a window of this program can be shown as two views side by side: the desktop takes declarations
// and, on Wayland, Qt gives the window no viewport of its own.
bool sideBySidePossible();
// Called once on the GUI thread after QGuiApplication exists; connects the declaration library on Wayland.
void initStereoDeclaration();

// Declares one QWindow as full side by side for as long as this object lives, and again whenever the window
// gets a new native surface. The window's picture must hold both eyes side by side, the left eye first.
class StereoDeclaration : public QObject
{
    Q_OBJECT
public:
    explicit StereoDeclaration(QWindow* window, QWindow* topLevel);
    ~StereoDeclaration() override;

    bool declared() const { return m_declared; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void declare();
    void remove();

    QWindow* m_window;
    QWindow* m_topLevel;
    bool m_declared = false;
};

} // namespace all::qt
