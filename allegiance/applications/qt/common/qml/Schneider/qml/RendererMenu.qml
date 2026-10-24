import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Schneider

// Which renderer draws the 3D area; under Advanced, since each gives a different picture only where the stack
// differs. The graphics API is Qt Quick's for the whole program, so a change applies at the next start.
GroupBox {
    id: renderer_menu
    objectName: "rendererSection"
    Layout.fillWidth: true
    title: qsTr("Renderer")
    label: Label {
        font: Style.fontDefault
        text: parent.title
    }

    function join(textStrs)
    {
        let str = "";
        for (let idx in textStrs) {
            str += textStrs[idx] + "\n";
        }
        return str
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right

        ComboBox {
            objectName: "rendererCombo"
            Layout.fillWidth: true
            model: [
                { value: Renderer.Engine.Quick3DVulkan, text: qsTr("Qt Quick 3D with Vulkan") },
                { value: Renderer.Engine.Quick3DOpenGL, text: qsTr("Qt Quick 3D with OpenGL") },
                { value: Renderer.Engine.Qt3DOpenGL, text: qsTr("Qt 3D with OpenGL") }
            ]
            textRole: "text"
            valueRole: "value"
            currentIndex: Renderer.engine
            onCurrentIndexChanged: {
                if (currentIndex !== -1 && Renderer.engine != currentIndex) {
                    Renderer.engine = currentIndex;
                }
            }
            ToolTip.visible: hovered
            ToolTip.text: join([qsTr("Qt Quick 3D with Vulkan: the scene through Qt Quick 3D on the Vulkan driver; each eye drawn by Qt Quick into its own target."),
                                qsTr("Qt Quick 3D with OpenGL: the same scene on OpenGL."),
                                qsTr("Qt 3D with OpenGL: the original demonstrator's renderer.")])
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            visible: Renderer.restartNeeded
            text: qsTr("Applies the next time the program starts")
            font: Style.fontDefault
        }
    }
}
