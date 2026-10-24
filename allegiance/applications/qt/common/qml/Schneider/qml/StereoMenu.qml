import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Schneider

ColumnLayout {
    id: stereo_menu
    objectName: "stereoSection"

    function join(textStrs)
    {
        let str = "";
        for (let idx in textStrs) {
            str += textStrs[idx] + "\n";
        }
        return str
    }

    CheckBoxX {
        Layout.fillWidth: true
        title: qsTr("Enable 3D")
        initial: Stereo.enabled
        onChecked: checkValue => Stereo.enabled = checkValue
        ToolTip.text: join([qsTr("Draws the two eyes and declares the 3D area to the desktop, which makes every screen's format from them."),
                            qsTr("Off: one picture, nothing declared.")])
    }

    RowLayout {
        Layout.fillWidth: true
        visible: Stereo.enabled
        Label {
            Layout.alignment: Qt.AlignLeft
            text: qsTr("Stereo output")
            font: Style.fontDefault
        }
        ComboBox {
            objectName: "stereoPathCombo"
            Layout.fillWidth: true
            model: [
                { value: Stereo.Path.Automatic, text: qsTr("Automatic") },
                { value: Stereo.Path.QuadBuffer, text: qsTr("Quad buffer") },
                { value: Stereo.Path.SideBySide, text: qsTr("Full side by side (declared)") }
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
            ToolTip.text: join([qsTr("Automatic: a stereo OpenGL context when the stack gives one, else one picture."),
                                qsTr("Quad buffer: each eye in its own back buffer; the display stack packs and declares them."),
                                qsTr("Full side by side (declared): both eyes drawn side by side by the program, declared to the desktop; for stacks without a stereo context.")])
        }
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Active: %1").arg(Stereo.activePath)
        font: Style.fontDefault
    }

    Label {
        Layout.fillWidth: true
        visible: Stereo.activeStereo
        text: qsTr("Render resolution per eye: %1 x %2").arg(Stereo.eyeSize.width).arg(Stereo.eyeSize.height)
        font: Style.fontDefault
        ToolTip.visible: labelMa.containsMouse
        ToolTip.text: qsTr("Each eye is drawn at the 3D area's size; the desktop resizes it once for each screen.")
        MouseArea {
            id: labelMa
            anchors.fill: parent
            hoverEnabled: true
        }
    }

    ButtonX {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        visible: Stereo.displaySettingsAvailable
        text: qsTr("Display settings")
        onClicked: Stereo.openDisplaySettings()
    }
}
