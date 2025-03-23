import QtQuick
import QtMultimedia
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Themes
import "widgets"

Rectangle {
    id: voxPlayer
    color: "#1E1E1E"

    property double defaultVolume: 0.7

    MediaPlayer {
        id: mediaPlayer
        Component.onCompleted: {
            vox_manager.set_player(this);
        }
        onAudioOutputChanged: mediaPlayer.audioOutput.volume = volumeSlider.value;
        onErrorOccurred: (errorCode, errorStr) => {
            console.log(`mediaPlayer error: ${errorStr} (code: ${errorCode})`);
        }
    }
    function playPause() {
        if (mediaPlayer.playbackState === MediaPlayer.PlayingState) {
            mediaPlayer.pause()
        } else {
            mediaPlayer.play()
        }
    }
    Connections {
        target: vox_manager
        function onIsReadyChanged() {
            langSelector.isFirstSelected = vox_manager.captionsLang1.length >= vox_manager.captionsLang2.length;
        }
    }

    Timer {
        interval: 100
        running: mediaPlayer.playbackState === MediaPlayer.PlayingState
        repeat: true
        onTriggered: voxPlayer.updateCaption()
    }
    function updateCaption() {
        if (langSelector.isFirstSelected) {
            var captions = vox_manager.captionsLang1;
            var trackChanges = vox_manager.trackChangesLang1;
        } else {
            var captions = vox_manager.captionsLang2;
            var trackChanges = vox_manager.trackChangesLang2;
        }
        // Update captions
        let currentTime = mediaPlayer.position;
        let caption = captions.find(s => currentTime >= s.time && currentTime <= (s.time + s.duration));
        if (caption) {
            captionText.text = caption.text;
        } else {
            captionText.text = "";
        }
        // Set current enabled character portrait
        if (charas.model === null) {
            return;
        }
        let track = trackChanges.find(s => currentTime >= s.time && currentTime <= (s.time + s.duration));
        if (track) {
            for (let i = 0; i < charas.model.length; i++) {
                let chara = charas.itemAt(i);
                chara.enabled = track.index === i+1;
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        property color backgroundColor: vox_manager.isReady ? "black" : Theme.color_background1
        color: backgroundColor
        radius: 10
        anchors.margins: 10

        // Allow opening files with drag'n drop
        DropArea {
            anchors.fill: parent
            onEntered: {
                parent.color = Theme.color_highlight
                charas.visible = false;
            }
            onExited: {
                charas.visible = true;
                parent.color = Qt.binding(() => {return parent.backgroundColor});
            }
            onDropped: function(drop) {
                parent.color = Qt.binding(() => {return parent.backgroundColor});
                if (drop.urls.length > 0) {
                    vox_manager.open_vox_file(drop.urls[0]);
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent

            // Draw 2 to 4 character faces
            RowLayout {
                spacing: 30
                Layout.alignment: Qt.AlignHCenter
                Layout.minimumHeight: charaHeight
                Layout.maximumHeight: charaHeight
                property int charaHeight: window.scaleHeight(89 * 2)
                property int charaWidth: window.scaleWidth(52 * 2)
                Repeater {
                    id: charas
                    model: vox_manager.charas.length > 1 ? vox_manager.charas : null
                    delegate: CodecChara {
                        name: vox_manager.charas[index]
                        Layout.preferredHeight: mediaPlayer.playbackState === MediaPlayer.StoppedState
                                                && mediaPlayer.position === mediaPlayer.duration
                                                    ? 0 : parent.charaHeight
                        // Manually keep image aspect ratio for stretch animation
                        Layout.preferredWidth: parent.charaHeight * (52 / 89)
                    }
                }
            }

            Text {
                id: captionText
                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                font.pixelSize: window.scaleHeight(20)
                color: 'white'
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                Layout.minimumHeight: window.scaleHeight(115)
                Layout.maximumHeight: window.scaleHeight(115)
                Layout.minimumWidth: parent.width
                Layout.maximumWidth: parent.width
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20

                Text {
                    text: vox_manager.isReady ? formatTime(mediaPlayer.position) : '--:--'
                    color: "#FFFFFF"
                    font.pixelSize: 14
                }

                // Progress slider
                Slider {
                    id: progressSlider
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    enabled: vox_manager.isReady
                    from: 0
                    to: mediaPlayer.duration
                    value: mediaPlayer.position
                    background: Rectangle {
                        x: progressSlider.leftPadding
                        y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                        implicitWidth: 200
                        implicitHeight: 4
                        width: progressSlider.availableWidth
                        height: implicitHeight
                        radius: 2
                        color: "#808080"
                        Rectangle {
                            width: progressSlider.visualPosition * parent.width
                            height: parent.height
                            color: vox_manager.isReady ? Theme.color_highlight : "#404040"
                            radius: 2
                        }
                    }
                    handle: Rectangle {
                        x: progressSlider.leftPadding + progressSlider.visualPosition * progressSlider.availableWidth - width / 2
                        y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                        implicitWidth: 14
                        implicitHeight: 14
                        radius: 7
                        color: vox_manager.isReady ? Theme.color_highlight : "#404040"
                    }
                    onMoved: {
                        mediaPlayer.position = value;
                        voxPlayer.updateCaption();
                    }
                }

                Text {
                    text: vox_manager.isReady ? formatTime(mediaPlayer.duration) : '--:--'
                    color: "#FFFFFF"
                    font.pixelSize: 14
                }
            }

            // Controls
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 20

                RowLayout {
                    id: controls
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 20

                    // Play/Pause button
                    RoundButton {
                        id: playButton
                        Layout.leftMargin: 5
                        implicitWidth: 40
                        implicitHeight: 40
                        enabled: vox_manager.isReady
                        Layout.alignment: Qt.AlignHCenter
                        radius: 25
                        background: Rectangle {
                            radius: 25
                            color: playButton.hovered && playButton.enabled ? Theme.color_highlight : Theme.color_background2
                            border.color: playButton.enabled ? Theme.color_highlight : Theme.color_buttonBorder
                        }
                        icon.source: mediaPlayer.playbackState === MediaPlayer.PlayingState
                                        ? "qrc:/logos/pause.svg"
                                        : "qrc:/logos/play.svg"
                        icon.color: playButton.enabled ? (playButton.hovered ? "white" : "#d2d2d2") : Theme.color_buttonBorder
                        onClicked: {
                            voxPlayer.playPause();
                        }
                        // Force focus so we can use SPACE button to play/pause
                        focus: true
                        onFocusChanged: focus = true
                    }

                    Item { Layout.fillWidth: true }

                    // Language selector
                    Rectangle {
                        Layout.minimumHeight: 22
                        Layout.maximumHeight: 22
                        Layout.minimumWidth: 110
                        Layout.maximumWidth: 110
                        border {
                            width: 2
                            //color: Theme.color_highlight
                            color: Theme.color_background1 // no border
                            //color: "gray"
                        }
                        radius: 30
                        RowLayout {
                            id: langSelector
                            //property bool isFirstSelected: false
                            property bool isFirstSelected: vox_manager.captionsLang1.length >= vox_manager.captionsLang2.length
                            enabled: vox_manager.captionsLang1.length > 0 || vox_manager.captionsLang2.length > 0
                            anchors.centerIn: parent
                            height: parent.height - parent.border.width
                            width: parent.width - parent.border.width
                            spacing: 0
                            // Button language 1
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: langSelector.isFirstSelected ? "#2f7200" : Theme.color_background1
                                radius: 30
                                Layout.alignment: Qt.AlignLeft
                                enabled: vox_manager.captionsLang1.length > 0
                                Rectangle {
                                    width: parent.radius
                                    height: parent.height
                                    anchors.right: parent.right
                                    color: parent.color
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: "Lang 1"
                                    font.pixelSize: 12
                                    color: parent.enabled ? (langSelector.isFirstSelected ? "white" : "#d2d2d2") : "gray"
                                    anchors.fill: parent
                                    horizontalAlignment: Text.AlignHCenter
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: vox_manager.captionsLang1.length > 0
                                    onClicked: {
                                        langSelector.isFirstSelected = true;
                                        voxPlayer.updateCaption();
                                    }
                                }
                            }
                            // Button language 2
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: !langSelector.isFirstSelected ? "#2f7200" : Theme.color_background1
                                radius: 30
                                Layout.alignment: Qt.AlignRight
                                enabled: vox_manager.captionsLang2.length > 0
                                Rectangle {
                                    width: parent.radius
                                    height: parent.height
                                    anchors.left: parent.left
                                    color: parent.color
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: "Lang 2"
                                    font.pixelSize: 12
                                    color: parent.enabled ? (!langSelector.isFirstSelected ? "white" : "#d2d2d2") : "gray"
                                    anchors.fill: parent
                                    horizontalAlignment: Text.AlignHCenter
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    //enabled: vox_manager.captionsLang2.length > 0
                                    onClicked: {
                                        langSelector.isFirstSelected = false;
                                        voxPlayer.updateCaption();
                                    }
                                }
                            }
                        }
                    }

                    // Volume control
                    RowLayout {
                        Item {
                            Layout.minimumWidth: 30
                            Layout.maximumWidth: 30
                            Layout.alignment: Qt.AlignRight | Qt.AlignTop
                            Image {
                                id: volumeControl
                                source: volumeSlider.value > 0.0 ? "qrc:/logos/volume_up.svg" : "qrc:/logos/volume_mute.svg"
                                MouseArea {
                                    anchors.fill: parent
                                    property double oldVolume: 100.0
                                    onClicked: {
                                        if (volumeSlider.value > 0) {
                                            oldVolume = volumeSlider.value;
                                            volumeSlider.value = 0;
                                        } else {
                                            volumeSlider.value = oldVolume;
                                        }
                                    }
                                }
                            }
                            MultiEffect {
                                source: volumeControl
                                anchors.fill: volumeControl
                                colorization: 1.0
                                colorizationColor: 'LightGray'
                            }
                        }
                        Slider {
                            id: volumeSlider
                            from: 0
                            to: 1
                            value: voxPlayer.defaultVolume
                            width: 100
                            background: Rectangle {
                                x: volumeSlider.leftPadding
                                y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                                implicitWidth: 100
                                implicitHeight: 4
                                width: volumeSlider.availableWidth
                                height: implicitHeight
                                radius: 2
                                color: "#404040"
                                Rectangle {
                                    width: volumeSlider.visualPosition * parent.width
                                    height: parent.height
                                    color: Theme.color_highlight
                                    radius: 2
                                }
                            }
                            handle: Rectangle {
                                x: volumeSlider.leftPadding + volumeSlider.visualPosition * volumeSlider.availableWidth - width / 2
                                y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                                implicitWidth: 12
                                implicitHeight: 12
                                radius: 6
                                color: Theme.color_highlight
                            }
                            onValueChanged: {
                                if (mediaPlayer.audioOutput !== null) {
                                    mediaPlayer.audioOutput.volume = value
                                }

                            }
                        }
                    }
                }
            }
            Item {
                Layout.preferredHeight: 5
            }
        }
    }

    function formatTime(ms) {
        let seconds = Math.floor(ms / 1000)
        let minutes = Math.floor(seconds / 60)
        seconds = seconds % 60
        return `${minutes}:${seconds < 10 ? "0" : ""}${seconds}`
    }
}
