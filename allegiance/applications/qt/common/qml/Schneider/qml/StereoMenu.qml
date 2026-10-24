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

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Active: %1").arg(Stereo.activePath)
        font: Style.fontDefault
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
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
