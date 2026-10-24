#include "focus_area.h"

#include <QVector2D>

#include <algorithm>
#include <cmath>

namespace all::quick3d {

FocusArea::FocusArea(QObject* parent)
    : QObject(parent)
{
}

void FocusArea::setCenter(QPointF center)
{
    if (m_center == center)
        return;
    m_center = center;
    Q_EMIT changed();
}

void FocusArea::setExtent(QSizeF extent)
{
    const QSizeF e(std::max(extent.width(), 50.0), std::max(extent.height(), 50.0));
    if (m_extent == e)
        return;
    m_extent = e;
    Q_EMIT changed();
}

void FocusArea::setViewSize(QSizeF size)
{
    if (m_viewSize == size)
        return;
    m_viewSize = size;
    // The area stays centred on the picture when the picture changes size
    setCenter(QPointF(size.width() * 0.5, size.height() * 0.5));
}

void FocusArea::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    if (!enabled && m_hover != Hover::None) {
        m_hover = Hover::None;
        Q_EMIT hoverChanged();
    }
    Q_EMIT changed();
}

FocusArea::Hover FocusArea::hoverAt(QPointF p) const
{
    if (!m_enabled)
        return Hover::None;
    // The resize handle: the triangle in the bottom right corner
    const QPointF a = m_center + QPointF(0.5 * m_extent.width(), 0.3 * m_extent.height());
    const QPointF b = m_center + QPointF(0.3 * m_extent.width(), 0.5 * m_extent.height());
    const QPointF c = m_center + QPointF(0.5 * m_extent.width(), 0.5 * m_extent.height());
    auto cross = [](QPointF u, QPointF v) { return u.x() * v.y() - u.y() * v.x(); };
    const bool s1 = cross(b - a, p - a) > 0.0;
    const bool s2 = cross(c - b, p - b) > 0.0;
    const bool s3 = cross(a - c, p - c) > 0.0;
    if (s1 == s2 && s2 == s3)
        return Hover::Resize;
    // The centre cross
    if (QVector2D(p - m_center).lengthSquared() < 20.0f * 20.0f)
        return Hover::Center;
    return Hover::None;
}

void FocusArea::mousePressed(QPointF p)
{
    m_distToCenterOnPress = m_center - p;
    m_extentOnPress = m_extent;
    if (m_hover == Hover::Center)
        m_operation = Operation::Translating;
    else if (m_hover == Hover::Resize)
        m_operation = Operation::Scaling;
}

void FocusArea::mouseMoved(QPointF p)
{
    if (m_operation == Operation::None) {
        const Hover hover = hoverAt(p);
        if (hover != m_hover) {
            m_hover = hover;
            Q_EMIT hoverChanged();
        }
    } else if (m_operation == Operation::Translating) {
        setCenter(m_distToCenterOnPress + p);
    } else if (m_operation == Operation::Scaling) {
        const double pressed = std::hypot(m_distToCenterOnPress.x(), m_distToCenterOnPress.y());
        const double now = std::hypot((m_center - p).x(), (m_center - p).y());
        if (pressed > 0.0)
            setExtent(m_extentOnPress * (now / pressed));
    }
}

void FocusArea::mouseReleased()
{
    m_operation = Operation::None;
}

} // namespace all::quick3d
