#pragma once
#include <QObject>
#include <QPointF>
#include <QSizeF>

class QMouseEvent;

namespace all::quick3d {

// The autofocus area over one eye's picture, in that picture's pixels: its centre and extent, moved by dragging
// its centre cross and resized by dragging its bottom right corner (the original demonstrator's interaction).
// The drawing is the scene's; this is the shape and the mouse.
class FocusArea : public QObject
{
    Q_OBJECT
public:
    enum class Hover {
        None,
        Center,
        Resize
    };

    explicit FocusArea(QObject* parent = nullptr);

    QPointF center() const { return m_center; }
    QSizeF extent() const { return m_extent; }
    void setCenter(QPointF center);
    void setExtent(QSizeF extent);
    void setViewSize(QSizeF size);
    QSizeF viewSize() const { return m_viewSize; }

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool containsMouse() const { return m_hover != Hover::None; }
    Hover hover() const { return m_hover; }

    // Positions in the eye's pixels
    void mousePressed(QPointF p);
    void mouseMoved(QPointF p);
    void mouseReleased();

    // Where the mouse is over the area's shape
    Hover hoverAt(QPointF p) const;

Q_SIGNALS:
    void changed();
    void hoverChanged();

private:
    enum class Operation {
        None,
        Translating,
        Scaling
    };
    QPointF m_center;
    QSizeF m_extent{ 100.0, 100.0 };
    QSizeF m_viewSize;
    bool m_enabled = false;
    Hover m_hover = Hover::None;
    Operation m_operation = Operation::None;
    QPointF m_distToCenterOnPress;
    QSizeF m_extentOnPress;
};

} // namespace all::quick3d
