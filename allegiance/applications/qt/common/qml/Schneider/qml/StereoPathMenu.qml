import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Schneider

// How the two eyes reach the desktop: a choice of route, never an output format (every screen's format is
// the desktop's); under Advanced, since each route gives a different picture only where the stack differs.
GroupBox {
    id: stereo_path_menu
    objectName: "stereoPathSection"
    Layout.fillWidth: true
    title: qsTr("Stereo 3D")
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

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("How the eyes reach the desktop")
            font: Style.fontDefault
        }
        ComboBox {
            objectName: "stereoPathCombo"
            Layout.fillWidth: true
            model: [
                { value: Stereo.Path.Automatic, text: qsTr("Automatic") },
                { value: Stereo.Path.QuadBuffer, text: qsTr("Quad buffer") },
                { value: Stereo.Path.SideBySide, text: qsTr("Two views side by side (declared)") }
            ]
            textRole: "text"
            valueRole: "value"
            currentIndex: Stereo.path
            onCurrentIndexChanged: {
                if (currentIndex !== -1 && Stereo.path != currentIndex) {
                    Stereo.path = currentIndex;
                }
            }
            ToolTip.visible: hovered
            ToolTip.text: join([qsTr("Automatic: a stereo OpenGL context when the stack gives one, else the two views side by side, declared."),
                                qsTr("Quad buffer: each eye in its own back buffer; the display stack packs and declares them."),
                                qsTr("Two views side by side (declared): both eyes drawn side by side by the program into one window and declared to the desktop; for stacks without a stereo context.")])
        }
    }
}
