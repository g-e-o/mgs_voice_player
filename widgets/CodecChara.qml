import QtQuick
import QtMultimedia
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import Themes

Item {
    id: chara
    property string name: ''
    Image {
        id: charaImage
        visible: vox_manager.charas.length > 0
        source: chara.name === null ? null : `qrc:/faces/${chara.name}.png`
        anchors.centerIn: parent
        height: 0
        width: parent.width
        Behavior on height { NumberAnimation { duration: 250; easing.type: Easing.OutQuad } }
    }
    onHeightChanged: charaImage.height = height
    Glow {
        anchors.fill: charaImage
        source: charaImage
        radius: 16
        samples: 16
        color: "#55adffd6"
        transparentBorder: true
        spread: 0.2
        opacity: chara.enabled ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }
    MultiEffect {
        source: charaImage
        anchors.fill: charaImage
        colorization: 1.0
        colorizationColor: chara.enabled ? '#adffd6' : 'Gray'
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }
    Canvas {
        id: tearCanvas
        opacity: chara.enabled && vox_manager.isReady ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
        anchors.fill: parent
        property real bandOffset: 0
        NumberAnimation on bandOffset {
            id: bandAnimation
            loops: Animation.Infinite
            from: 0
            to: height
            duration: 1
            running: true
        }
        Connections {
            target: mediaPlayer
            function onPlayingChanged() {
                if (mediaPlayer.playing) {
                    if (bandAnimation.paused) {
                        bandAnimation.resume();
                    }
                } else {
                    bandAnimation.pause();
                }
            }
        }
        onPaint: {
            // Draw horizontal tearing bands
            var ctx = getContext("2d");
            ctx.reset();
            ctx.fillStyle = "rgba(90, 158, 95, 0.3)";
            let bandsCount = 2;
            for (var i = 0; i < bandsCount; i++) {
                let yPos = (bandOffset + i * height / bandsCount) % height;
                // Avoid having adjacent chara tearing bands aligned, it looks weird..
                let shiftPos = index % 2 == 0 ? 40 : 0;
                ctx.fillRect(0, yPos + shiftPos, width, 10 * (i+1));
                bandAnimation.duration = 10000;
            }
        }
        onBandOffsetChanged: {
            tearCanvas.requestPaint();
        }
    }
}
