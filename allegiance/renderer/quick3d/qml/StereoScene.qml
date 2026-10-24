import QtQuick
import QtQuick3D
import QtQuick3D.AssetUtils
import Stereo3D.Quick3D

// The 3D area drawn by Qt Quick 3D: one scene (the model, the lights, the 3D cursor, the focus plane) seen by the
// left, right and centre cameras, which the renderer places from the shared stereo camera. One View3D per eye on
// the declared pair path (the window twice as wide); one View3D on the quad-buffer path, whose camera follows the
// eye the edition's Qt Quick is drawing (currentEye). The focus area and the frustum viewer are overlays at screen
// depth, drawn once per eye. The two eye views exist from the start (a View3D made or unmade while another imports
// the same scene brings Qt Quick 3D down); the right one shows only for the pair.
Item {
    id: root
    objectName: "stereoScene"

    // Set by the renderer
    property int views: 1
    property int eyeWidth: 640
    property int eyeHeight: 480
    property int currentEye: 0 // 0: left (or the one picture), 1: right; the quad-buffer pass being drawn
    property int displayMode: 0 // all::DisplayMode: 0 stereo, 1 mono, 2 left, 3 right
    property bool wireframe: false
    property color clearColor: "#48536a"
    property url modelSource
    property bool imageMode: false
    property url imageSource
    property bool cursorVisible: true
    property int cursorType: 0 // all::CursorType: 0 ball, 1 cross, 2 cross hair, 3 dot
    property color cursorColor: "#ffffff"
    property real cursorScale: 0.06
    property bool showFocusArea: false
    property bool showFocusPlane: false
    property bool showFrustum: false
    property real focusAreaCenterX: eyeWidth / 2
    property real focusAreaCenterY: eyeHeight / 2
    property real focusAreaWidth: 100
    property real focusAreaHeight: 100
    property int focusAreaHover: 0 // 0 none, 1 centre, 2 resize
    property color focusAreaColor: "#ffd040"
    property color focusPlaneColor: "#4040ff80"

    width: eyeWidth * views
    height: eyeHeight

    function cameraForEye(eye) {
        if (displayMode === 1)
            return centerCamera;
        if (displayMode === 2)
            return leftCamera;
        if (displayMode === 3)
            return rightCamera;
        return eye === 1 ? rightCamera : leftCamera;
    }

    // The eye a view shows: the left view shows the pass being drawn on the quad-buffer path
    function eyeOfView(index) {
        return views === 2 ? index : currentEye;
    }

    Node {
        id: sceneRoot
        objectName: "sceneRoot"

        CustomCamera { id: leftCamera; objectName: "leftCamera" }
        CustomCamera { id: rightCamera; objectName: "rightCamera" }
        CustomCamera { id: centerCamera; objectName: "centerCamera" }

        // The lights of the original demonstrator: four directions (the renderer turns them), no shadows
        DirectionalLight { objectName: "light0"; brightness: 0.5 }
        DirectionalLight { objectName: "light1"; brightness: 0.35 }
        DirectionalLight { objectName: "light2"; brightness: 0.5 }
        DirectionalLight { objectName: "light3"; brightness: 0.35; ambientColor: "#303030" }

        Node {
            id: userRoot
            objectName: "userRoot"
            visible: !root.imageMode
            RuntimeLoader {
                id: loader
                objectName: "loader"
                source: root.modelSource
            }
        }

        // The 3D cursor: a ball (a sphere with its billboard), a cross, a cross hair or a dot
        Node {
            id: cursorNode
            objectName: "cursorNode"
            visible: root.cursorVisible && !root.imageMode
            scale: Qt.vector3d(root.cursorScale, root.cursorScale, root.cursorScale)
            Model {
                objectName: "cursorSphere"
                visible: root.cursorType === 0
                source: "#Sphere"
                scale: Qt.vector3d(0.02, 0.02, 0.02) // a unit radius
                materials: PrincipledMaterial {
                    baseColor: root.cursorColor
                    roughness: 0.4
                }
            }
            Node {
                id: cursorBillboard
                objectName: "cursorBillboard"
                visible: root.cursorType !== 1
                Model {
                    source: "#Rectangle"
                    scale: Qt.vector3d(0.02, 0.02, 0.02) // two units wide
                    materials: PrincipledMaterial {
                        lighting: PrincipledMaterial.NoLighting
                        baseColor: root.cursorColor
                        baseColorMap: Texture {
                            source: root.cursorType === 2 ? "qrc:/cursor_billboard_crosshair.png" : root.cursorType === 3 ? "qrc:/cursor_billboard_dot.png" : "qrc:/cursor_billboard.png"
                        }
                        alphaMode: PrincipledMaterial.Blend
                        cullMode: Material.NoCulling
                    }
                }
            }
            Node {
                objectName: "cursorCross"
                visible: root.cursorType === 1
                Model { source: "#Cube"; scale: Qt.vector3d(0.06, 0.002, 0.002); materials: crossMaterial }
                Model { source: "#Cube"; scale: Qt.vector3d(0.002, 0.06, 0.002); materials: crossMaterial }
                Model { source: "#Cube"; scale: Qt.vector3d(0.002, 0.002, 0.06); materials: crossMaterial }
            }
        }
        PrincipledMaterial {
            id: crossMaterial
            baseColor: "lightyellow"
            roughness: 0.2
        }

        // The focus plane preview: a translucent quad at the convergence distance, filling the centre camera's view
        Model {
            id: focusPlane
            objectName: "focusPlane"
            visible: root.showFocusPlane && !root.imageMode
            source: "#Rectangle"
            materials: PrincipledMaterial {
                lighting: PrincipledMaterial.NoLighting
                baseColor: root.focusPlaneColor
                alphaMode: PrincipledMaterial.Blend
                cullMode: Material.NoCulling
            }
        }
    }

    // The frustum viewer's scene: the two eyes' frusta (amplified) and the convergence plane, seen from above
    Node {
        id: frustumRoot
        objectName: "frustumRoot"
        CustomCamera { id: frustumCamera; objectName: "frustumCamera" }
        Model {
            geometry: FrustumGeometry { objectName: "leftFrustumLines"; primitive: FrustumGeometry.Lines }
            materials: PrincipledMaterial { lighting: PrincipledMaterial.NoLighting; baseColor: "#ff8080" }
        }
        Model {
            geometry: FrustumGeometry { objectName: "rightFrustumLines"; primitive: FrustumGeometry.Lines }
            materials: PrincipledMaterial { lighting: PrincipledMaterial.NoLighting; baseColor: "#0080ff" }
        }
        Model {
            geometry: FrustumGeometry { objectName: "leftFrustumPlane"; primitive: FrustumGeometry.Triangles }
            materials: PrincipledMaterial { lighting: PrincipledMaterial.NoLighting; baseColor: "#90ff8080"; alphaMode: PrincipledMaterial.Blend; cullMode: Material.NoCulling }
        }
        Model {
            geometry: FrustumGeometry { objectName: "rightFrustumPlane"; primitive: FrustumGeometry.Triangles }
            materials: PrincipledMaterial { lighting: PrincipledMaterial.NoLighting; baseColor: "#900080ff"; alphaMode: PrincipledMaterial.Blend; cullMode: Material.NoCulling }
        }
    }

    // The whole side-by-side picture's size, read apart from the clipped halves
    Image {
        id: imageProbe
        visible: false
        source: root.imageSource
    }

    // One eye's picture: its view of the scene, or its half of a side-by-side picture, with the overlays
    component EyeView: Item {
        id: eyeItem
        required property int eyeIndex
        property int shownEye: root.eyeOfView(eyeIndex)
        x: eyeIndex * root.eyeWidth
        y: 0
        width: root.eyeWidth
        height: root.eyeHeight

        View3D {
            objectName: eyeItem.eyeIndex === 0 ? "leftView" : "rightView"
            anchors.fill: parent
            visible: !root.imageMode
            importScene: sceneRoot
            camera: root.cameraForEye(eyeItem.shownEye)
            environment: SceneEnvironment {
                clearColor: root.clearColor
                backgroundMode: SceneEnvironment.Color
                antialiasingMode: SceneEnvironment.MSAA
                antialiasingQuality: SceneEnvironment.High
                debugSettings: DebugSettings { wireframeEnabled: root.wireframe }
            }
        }

        // A side-by-side picture: each eye shows its half, zoomed as much as possible with its aspect kept
        Rectangle {
            anchors.fill: parent
            visible: root.imageMode
            color: root.clearColor
            Image {
                objectName: eyeItem.eyeIndex === 0 ? "leftImage" : "rightImage"
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: root.imageSource
                property int fullWidth: imageProbe.implicitWidth > 0 ? imageProbe.implicitWidth : 2
                property int fullHeight: imageProbe.implicitHeight > 0 ? imageProbe.implicitHeight : 1
                sourceClipRect: Qt.rect(eyeItem.shownEye === 1 ? fullWidth / 2 : 0, 0, fullWidth / 2, fullHeight)
            }
        }

        // The focus area: its frame, the centre cross that moves it and the corner handle that resizes it
        Item {
            visible: root.showFocusArea && !root.imageMode
            x: root.focusAreaCenterX - root.focusAreaWidth / 2
            y: root.focusAreaCenterY - root.focusAreaHeight / 2
            width: root.focusAreaWidth
            height: root.focusAreaHeight
            Rectangle { anchors.fill: parent; color: "transparent"; border.color: root.focusAreaColor; border.width: 1 }
            Rectangle { x: parent.width / 2 - 10; y: parent.height / 2; width: 20; height: 1; color: root.focusAreaHover === 1 ? "#ffffff" : root.focusAreaColor }
            Rectangle { x: parent.width / 2; y: parent.height / 2 - 10; width: 1; height: 20; color: root.focusAreaHover === 1 ? "#ffffff" : root.focusAreaColor }
            Canvas {
                anchors.fill: parent
                property color handleColor: root.focusAreaHover === 2 ? "#ffffff" : root.focusAreaColor
                onHandleColorChanged: requestPaint()
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                onPaint: {
                    const ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);
                    ctx.fillStyle = handleColor;
                    ctx.beginPath();
                    ctx.moveTo(width, height * 0.8);
                    ctx.lineTo(width * 0.8, height);
                    ctx.lineTo(width, height);
                    ctx.closePath();
                    ctx.fill();
                }
            }
        }

        // The frustum viewer, in the top left corner
        View3D {
            objectName: eyeItem.eyeIndex === 0 ? "leftFrustumView" : "rightFrustumView"
            visible: root.showFrustum && !root.imageMode
            x: 0
            y: 0
            width: parent.width * 0.4
            height: parent.height * 0.4
            importScene: frustumRoot
            camera: frustumCamera
            environment: SceneEnvironment {
                clearColor: "#30000000"
                backgroundMode: SceneEnvironment.Transparent
            }
        }
    }

    EyeView { objectName: "leftEye"; eyeIndex: 0 }
    EyeView { objectName: "rightEye"; eyeIndex: 1; visible: root.views === 2 }
}
