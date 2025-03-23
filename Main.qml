import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Themes
import 'widgets'

ApplicationWindow {
    id: window
    title: qsTr('MGS1 Voice Player 0.1')
    width: 600
    height: 500
    minimumWidth: 400
    minimumHeight: 450
    visible: true

    function scaleWidth(value) {
        return (window.width / 600) * value;
    }
    function scaleHeight(value) {
        return (window.height / 500) * value;
    }

    header: Rectangle {
        Layout.fillWidth: true
        height: 50
        color: "#2D2D2D"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            FileDialog {
                id: openFileDialog
                title: "Select Voice or Demo file"
                nameFilters: ["Voice|Demo file (*.vox *.dmo)"]
                onAccepted: {
                    vox_manager.open_vox_file(openFileDialog.selectedFile);
                }
            }
            FileDialog {
                id: saveFileDialog
                title: "Export sound or subtitle file"
                fileMode: FileDialog.SaveFile
                nameFilters: ["Wav|Vag|Srt file (*.wav *.vag *.srt)"]
                onAccepted: {
                    let filename = saveFileDialog.selectedFile.toString();
                    if (filename.endsWith(".vag")) {
                        vox_manager.export_vag(saveFileDialog.selectedFile);
                    } else if (filename.endsWith(".srt")) {
                        vox_manager.export_srt(saveFileDialog.selectedFile);
                    } else {
                        vox_manager.export_wav(saveFileDialog.selectedFile);
                    }
                }
            }

            RoundButton {
                id: openFileButton
                text: "Open"
                implicitWidth: 100
                implicitHeight: 30
                icon.source: "qrc:/logos/file_open.svg"
                icon.color: enabled ? (openFileButton.hovered ? "white" : Theme.color_highlight) : Theme.color_buttonBorder
                palette.buttonText: enabled ? Theme.color_buttonText : Theme.color_buttonBorder // text color
                background: Rectangle {
                    color: openFileButton.hovered ? Theme.color_highlight : Theme.color_background2
                    radius: 5
                    border.color: openFileButton.enabled ? Theme.color_highlight : Theme.color_buttonBorder
                }
                onClicked: openFileDialog.open()
            }
            RoundButton {
                id: saveFileButton
                text: "Export"
                implicitWidth: 100
                implicitHeight: 30
                enabled: vox_manager.isReady
                icon.source: "qrc:/logos/save.svg"
                icon.color: enabled ? (saveFileButton.hovered ? "white" : Theme.color_highlight) : Theme.color_buttonBorder
                palette.buttonText: enabled ? Theme.color_buttonText : Theme.color_buttonBorder // text color
                background: Rectangle {
                    color: saveFileButton.hovered && saveFileButton.enabled ? Theme.color_highlight : Theme.color_background2
                    radius: 5
                    border.color: saveFileButton.enabled ? Theme.color_highlight : Theme.color_buttonBorder
                }
                onClicked: saveFileDialog.open()
            }

            Item { Layout.fillWidth: true }
        }

    }
    VoxPlayer {
        anchors.fill: parent
    }
    footer: Rectangle {
        color: "#2D2D2D"
        height: 24
        RowLayout {
            visible: vox_manager.isReady
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            property int statusTextSize: 14
            Text {
                Layout.alignment: Qt.AlignLeft
                text: vox_manager.fileName
                font.pixelSize: parent.statusTextSize
                color: "#f1fffd"
            }
            Text {
                Layout.alignment: Qt.AlignRight
                text: vox_manager.sampleRate + "Hz/" + (vox_manager.channels < 2 ? "Mono" : "Stereo")
                font.pixelSize: parent.statusTextSize
                color: '#f1fffd'
            }
        }
    }
}
