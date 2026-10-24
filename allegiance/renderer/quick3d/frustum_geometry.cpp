#include "frustum_geometry.h"

#include <QByteArray>

#include <limits>

namespace all::quick3d {

FrustumGeometry::FrustumGeometry(QQuick3DObject* parent)
    : QQuick3DGeometry(parent)
{
    rebuild();
}

void FrustumGeometry::setPrimitive(Primitive primitive)
{
    if (m_primitive == primitive)
        return;
    m_primitive = primitive;
    Q_EMIT primitiveChanged();
    rebuild();
}

void FrustumGeometry::setVertices(const std::vector<QVector3D>& vertices)
{
    m_vertices = vertices;
    rebuild();
}

void FrustumGeometry::rebuild()
{
    clear();
    QByteArray data;
    data.resize(int(m_vertices.size() * 3 * sizeof(float)));
    auto* f = reinterpret_cast<float*>(data.data());
    QVector3D min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    QVector3D max = -min;
    for (const QVector3D& v : m_vertices) {
        *f++ = v.x();
        *f++ = v.y();
        *f++ = v.z();
        min = QVector3D(std::min(min.x(), v.x()), std::min(min.y(), v.y()), std::min(min.z(), v.z()));
        max = QVector3D(std::max(max.x(), v.x()), std::max(max.y(), v.y()), std::max(max.z(), v.z()));
    }
    setVertexData(data);
    setStride(3 * sizeof(float));
    setPrimitiveType(m_primitive == Primitive::Lines ? QQuick3DGeometry::PrimitiveType::Lines : QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0, QQuick3DGeometry::Attribute::F32Type);
    if (!m_vertices.empty())
        setBounds(min, max);
    update();
}

} // namespace all::quick3d
