/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QtCore
import "qml"
import ImxVideoToTexture

ApplicationWindow {
    id: mainWindow
    visible: true
    title: qsTr("i.MX Video To Texture Demo")
    color: Style.nxpGrey
    height: Screen.desktopAvailableHeight * 0.8
    width: Screen.desktopAvailableWidth * 0.95
    property int fullscreen: 0

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action { 
                text: qsTr("&Open File...")
                onTriggered: fileDialog.open()
            }
            Action { 
                text: qsTr("Open &Folder...")
                onTriggered: folderDialog.open()
            }
            MenuSeparator { }
            Action { 
                text: qsTr("&Quit")
                onTriggered: mainWindow.close()
            }
        }
        Menu {
            title: qsTr("&Help")
            Action { 
                text: qsTr("&Readme")
                onTriggered: readme.open()
            }
            Action { 
                text: qsTr("&Licenses")
                onTriggered: licenses.open()
            }
        }
    }

    MediaThumbnails { // Thumbnails
        id: mediathumbnails
        visible: (height < 50 || width < 50 ? false : true)
        width: parent.width * 0.2
        anchors.top: parent.top
        anchors.bottom: mediacontrols.top
        anchors.left: parent.left
        anchors.margins: Style.margins
        clip: true
        onFileSelected: {
            mediastream.source = selectedFile;
        }
    }

    Rectangle { // Media Stream Area
        id: mediastreamarea
        visible: (height < 50 || width < 50 ? false : true)
        anchors.top: parent.top
        anchors.bottom: (fullscreen ? parent.bottom : mediacontrols.top)
        anchors.right: parent.right
        anchors.left: (fullscreen ? parent.left : mediathumbnails.right)
        anchors.margins: (fullscreen ? 0 : Style.margins)
        property real ratio: width / height
        color: Style.nxpDarkGrey
        MediaStream {
            id: mediastream
            width: (ratio > parent.ratio ? parent.width : parent.height * ratio)
            height : (ratio > parent.ratio ? parent.width / ratio : parent.height)
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    MediaControls {
        id: mediacontrols
        stream: mediastream
        thumbnails: mediathumbnails
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }

    FileDialog {
        id: fileDialog
        title: "Open file"
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        onAccepted: {
            mediastream.source = selectedFile;
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Open folder"
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        onAccepted: {
            mediathumbnails.folder = selectedFolder;
        }
    }

    Popup {
        id: readme
        anchors.centerIn: Overlay.overlay
        width: 660
        height: 320
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Text {
            id: readmeheader
            text: "i.MX Video To Texture"
            width: parent.width
            height: 40
            font.pointSize: 16.0
            horizontalAlignment: Text.AlignHCenter

        }
        Text {
            id: readmetext
            text: "<br>This sample application shows how to render a GStreamer stream into an OpenGL-ES texture 
that can be used in user application with no buffer copy. 
The GUI application is based on Qt framework 
but this can be ported to any GUI framework supporting custom rendering of OpenGL texture. 
In this sample, the GStreamer pipeline uses <i>glupload</i> element to render the stream into an OpenGL texture. 
Mapping the OpenGL texture to the video buffer is done in GStreamer plugin using Linux dma-buf.
The application retrieves the OpenGL texture ID used by the GStreamer pipeline and uses it in an OpenGL context.
EGL context is created by Qt framework and provided to GStreamer so that it can be used by GStreamer to create the OpenGL texture. 
GStreamer pipeline uses an <i>appsink</i> element to retrieve the sink in the application.<br><br>
Test Patterns from GStreamer can be played from the left panel for ease of use.
Using <i>File/Open Folder</i> menu, user can also select a folder containing video files that will be shown in the Video section.
For simplicity sake, only <i>mkv</i> and <i>mp4</i> files are supported in this demo.
But source code, and especially GstPlayer module, can be reused to play any pipeline supported by the device."

            anchors.top: readmeheader.bottom
            wrapMode: Text.WordWrap
            font.pointSize: 10.0
            width: parent.width
            horizontalAlignment: Text.AlignLeft
        }
    }

    Popup {
        id: licenses
        anchors.centerIn: Overlay.overlay
        width: 660
        height: 180
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Text {
            id: licensesheader
            text: "Licenses"
            width: parent.width
            height: 40
            font.pointSize: 16.0
            horizontalAlignment: Text.AlignHCenter

        }
        Text {
            id: licenseslist
            text: "- <b>Qt</b>: This application uses the Open Source version of Qt libraries under <b>LGPL-3.0-only</b> license.<br>
- <b>GStreamer</b>: This application uses GStreamer libraries under <b>LGPL-2.1-only</b> license.<br>
- <b>UI Icons</b>: This application uses icons from Google Fonts under <b>Apache-2.0</b> license.<br>
- <b>Source code</b>: Sources files are provided under <b>BSD 3-Clause</b> license."

            anchors.top: licensesheader.bottom
            font.pointSize: 10.0
            width: parent.width
            horizontalAlignment: Text.AlignLeft
        }
    }
}
