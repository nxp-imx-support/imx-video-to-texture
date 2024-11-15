/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

import QtQuick
import QtQuick.Controls

Item {
    id: controls
    height: 55
    property var stream
    property var thumbnails

    Row { // Buttons
        id: button
        height: 20
        anchors.bottom: progressBar.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: Style.margins
        spacing: 6

        RoundButton {
            id: previousVideoButton
            text: "Previous"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/previous"
            onClicked: {
                thumbnails.previous()
            }
        }

        RoundButton {
            id: skipBackwardButton
            text: "10s <<"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/backward_10"
            onClicked: {
                stream.skip(-10)
            }
        }

        RoundButton  {
            id: playPauseButton
            text: "Pause"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/pause"
            onClicked: {
                if (playPauseButton.text == "Pause") {
                    stream.pause()
                    playPauseButton.text = "Play"
                    icon.source = "qrc:/icons/play"
                }
                else {
                    stream.play()
                    playPauseButton.text = "Pause"
                    icon.source = "qrc:/icons/pause"
                }
            } 
        }

        RoundButton {
            id: skipForwardButton
            text: ">> 10s"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/forward_10"
            onClicked: {
                stream.skip(10)
            }
        }

        RoundButton {
            id: nextVideoButton
            text: "Next"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/next"
            onClicked: {
                thumbnails.next()
            }
        }

        RoundButton {
            id: loopButton
            text: "loop"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/repeat"
            down: stream.looping

            onClicked: {
                stream.toggleLoop()
                if (playPauseButton.text == "Play") {
                    stream.play()
                    stream.pause()
                    playPauseButton.text = "Play"
                    playPauseButton.icon.source = "qrc:/icons/play"
                }
                if (playPauseButton.text == "Pause") {
                    stream.pause()
                    stream.play()
                    playPauseButton.text = "Pause"
                    playPauseButton.icon.source = "qrc:/icons/pause"
                }
            }
        }

        RoundButton {
            id: fullscreenButton
            text: "Fullscreen"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/fullscreen"
            down: fullscreen
            onClicked: {
                fullscreen = 1 - fullscreen
                if (fullscreen) {
                    mainWindow.showFullScreen()
                }
                else {
                    mainWindow.showMaximized()
                }
                thumbnails.opacity = 1 - fullscreen
                mainWindow.menuBar.visible = 1 - fullscreen
            }
        }
    }

    Slider { // Progress Bar
        id: progressBar
        width: parent.width * 0.9
        height: 25
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        value: stream.position
        onMoved: {
            stream.position = progressBar.value
        }
        background: Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.availableWidth
            height: 6
            color: Style.nxpDarkGrey
            border.color: Style.nxpGrey
            border.width: 1
            radius: 6
        }
    }
}