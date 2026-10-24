#pragma once
#include <QtQuick3D/QQuick3DGeometry>
#include <QVector3D>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace all::quick3d {

// A line or triangle set given as a list of positions, for the frustum viewer (the frusta's edges and the
// convergence planes)
class FrustumGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(Primitive primitive READ primitive WRITE setPrimitive NOTIFY primitiveChanged)
    QML_NAMED_ELEMENT(FrustumGeometry)

public:
    enum class Primitive {
        Lines,
        Triangles
    };
    Q_ENUM(Primitive)

    explicit FrustumGeometry(QQuick3DObject* parent = nullptr);

    Primitive primitive() const { return m_primitive; }
    void setPrimitive(Primitive primitive);

    void setVertices(const std::vector<QVector3D>& vertices);

Q_SIGNALS:
    void primitiveChanged();

private:
    void rebuild();

    Primitive m_primitive = Primitive::Lines;
    std::vector<QVector3D> m_vertices;
};

} // namespace all::quick3d
