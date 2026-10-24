import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Schneider

ScrollView {
    id: mainMenuRoot
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.horizontal.interactive: false

    ScrollBar.vertical.policy: ScrollBar.AlwaysOn
    ScrollBar.vertical.interactive: true

    // Set custom Font for the ToolTips
    readonly property font myFont: ({
         family: "Open Sans",
         pointSize: 12,
     })
    ToolTip.toolTip.font: myFont

    ColumnLayout {
        width: root.width - 10

        ButtonX {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            text: "Load Model"
            onClicked: {
                Scene.OpenLoadModelDialog()
            }
        }

        // Stereo 3D: the program's own options; the desktop makes every screen's format from the two eyes
        Pane {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            background: Rectangle {
                color: Style.background
            }

            RowLayout {
                SectionTitle {
                    Layout.maximumWidth: 40
                    Layout.alignment: Qt.AlignRight
                    id: stereo_section_title
                }
                Label {
                    text: qsTr("Stereo 3D")
                    font: Style.fontButton
                    leftPadding: 10
                }
            }
            topPadding: 0
            bottomPadding: 0
        }
        StereoMenu {
            Layout.fillWidth: true
            visible: stereo_section_title.isVisible
        }

        // Advanced: the demonstrator's own controls
        Pane {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            background: Rectangle {
                color: Style.background
            }

            RowLayout {
                SectionTitle {
                    Layout.maximumWidth: 40
                    Layout.alignment: Qt.AlignRight
                    id: advanced_section_title
                    isVisible: false
                }
                Label {
                    text: qsTr("Advanced")
                    font: Style.fontButton
                    leftPadding: 10
                }
            }
            topPadding: 0
            bottomPadding: 0
        }

        ColumnLayout {
        objectName: "advancedSection"
        Layout.fillWidth: true
        visible: advanced_section_title.isVisible

        // How the eyes reach the desktop
        StereoPathMenu {
            Layout.fillWidth: true
        }

        // Misc
        Pane {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            background: Rectangle {
                color: Style.background
            }

            RowLayout {
                SectionTitle {
                    Layout.maximumWidth: 40
                    Layout.alignment: Qt.AlignRight
                    id: misc_section_title
                }
                Label {
                    text: "Misc"
                    font: Style.fontButton
                    leftPadding: 10
                }
            }
            topPadding: 0
            bottomPadding: 0
        }
        MiscMenu {
            Layout.fillWidth: true
            visible: misc_section_title.isVisible
        }

        // Camera
        Pane {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            background: Rectangle {
                color: Style.background
            }

            RowLayout {
                SectionTitle {
                    Layout.maximumWidth: 40
                    Layout.alignment: Qt.AlignRight
                    id: camera_section_title
                }
                Label {
                    text: "Camera"
                    font: Style.fontButton
                    leftPadding: 10
                }
            }
            topPadding: 0
            bottomPadding: 0
        }
        CameraMenu {
            Layout.fillWidth: true
            visible: camera_section_title.isVisible
        }

        // Cursor
        Pane {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            background: Rectangle {
                color: Style.background
            }

            RowLayout {
                SectionTitle {
                    Layout.maximumWidth: 40
                    Layout.alignment: Qt.AlignRight
                    id: cursor_section_title
                }
                Label {
                    text: "Cursor"
                    font: Style.fontButton
                    leftPadding: 10
                }
            }
            topPadding: 0
            bottomPadding: 0
        }
        CursorMenu {
            visible: cursor_section_title.isVisible
        }

        // Navigation
        Pane {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            background: Rectangle {
                color: Style.background
            }

            RowLayout {
                SectionTitle {
                    Layout.maximumWidth: 40
                    Layout.alignment: Qt.AlignRight
                    id: navigation_section_title
                }
                Label {
                    text: "Navigation"
                    font: Style.fontButton
                    leftPadding: 10
                }
            }
            topPadding: 0
            bottomPadding: 0
        }

        GroupBox {
            id: navigation
            visible: navigation_section_title.isVisible
            Layout.fillWidth: true
            title: "Navigation"
            label: CameraMenu.GroupBoxTitleLabel{}


            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right

                SliderValue {
                    id: mousesensitivity
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    from: 20
                    to: 300
                    value: Scene.mouseSensitivity
                    title: "Mouse Sensitivity"

                    onMoved: current => Scene.mouseSensitivity = current
                }

                SliderValue {
                    title: "Zoom Speed"
                    from: 1
                    to: 25
                    Layout.fillWidth: true
                    value: Scene.zoomAmount
                    defaultValue: Scene.defaultZoomAmount
                    onMoved: current => Scene.zoomAmount = Math.round(current)
                }

                /*      Still needs to be implemented on Application Side
                            Item {
                                Layout.fillWidth: true
                                height: 30
                                anchors.margins: 10
                                Button {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.topMargin: 10
                                    anchors.bottomMargin: 10
                                    text: "Reset Pivot Point"
                                }
                                Button {
                                    anchors.top: parent.top
                                    anchors.right: parent.right
                                    anchors.topMargin: 10
                                    anchors.bottomMargin: 10
                                    text: "Reset Camera Position"
                                }
                            }
                 */
            }
        }
        } // Advanced
    }
}
