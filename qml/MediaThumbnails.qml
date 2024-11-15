/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

import QtQuick
import QtQuick.Layouts
import Qt.labs.folderlistmodel
import Qt.labs.platform
import ImxVideoToTexture

Item {
    id: thumbnails
    property alias folder: videomodel.folder
    property string selectedFile: ""
    property int cellWidth: thumbnails.width * 0.5
    property int cellHeight: thumbnails.width * 0.5
    property int focusedGrid: 0
    signal fileSelected

    function getFocusedGridItem() {
        if (focusedGrid == 0) {
            return patternthumbnail.item
        }
        else {
            return videothumbnail.item
        }
    }

    function next() {
        let item = getFocusedGridItem()
        item.moveCurrentIndexRight()
        item.select(item.currentItem)
    }

    function previous() {
        let item = getFocusedGridItem()
        item.moveCurrentIndexLeft()
        item.select(item.currentItem)
    }

    ListModel {
        id: testpatternmodel
        ListElement {
            fileName: "smpte"
            fileUrl: "testbin://video,pattern=smpte,caps=[video/x-raw,format=BGRA]"
        }
        ListElement {
            fileName: "ball"
            fileUrl: "testbin://video,pattern=ball,caps=[video/x-raw,format=BGRA]"
        }
        ListElement {
            fileName: "snow"
            fileUrl: "testbin://video,pattern=snow,caps=[video/x-raw,format=BGRA]"
        }
    }

    FolderListModel {
        id: videomodel
        folder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        showDirs: false
        nameFilters: ["*.mkv", "*.mp4"]
    }

    Component {
        id: thumbnailsdelegate
        Item {
            id: thumbnailsitem
            required property string fileName
            required property string fileUrl
            width: thumbnails.cellWidth
            height: thumbnails.cellHeight
            Item {
                id: thumbnailsimage
                width: parent.width * 0.85
                height: parent.height * 0.85
                anchors.top: parent.top
                anchors.topMargin: 6
                property real ratio: width / height
                anchors.horizontalCenter : parent.horizontalCenter
                Image {
                    visible: 1 - mediascreenshot.loaded
                    width: parent.width
                    height: parent.height
                    anchors.verticalCenter : parent.verticalCenter
                    source: "qrc:/image/videothumbnail";
                }
                MediaScreenshot {
                    id: mediascreenshot
                    width: (ratio > parent.ratio ? parent.width : parent.height * ratio)
                    height : (ratio > parent.ratio ? parent.width / ratio : parent.height)
                    anchors.horizontalCenter : parent.horizontalCenter
                    anchors.bottom : parent.bottom
                    source: fileUrl
                    atPercent: 0.2
                }
            }
            Text {
                id: thumbnailstext
                text: fileName
                anchors.top: thumbnailsimage.bottom
                anchors.bottom: parent.bottom
                font.pointSize: 10.0
                font.underline: true
                width: parent.width * 0.85
                anchors.horizontalCenter : parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                clip: true
            }
        }
    }

    Component {
        id: thumbnailsview
        GridView {
            id: thumbnailsgrid
            width: parent.width
            height: parent.height
            cellWidth: thumbnails.cellWidth
            cellHeight: thumbnails.cellHeight
            clip: true
            keyNavigationWraps: true
            model: vmodel
            delegate: thumbnailsdelegate
            cacheBuffer: parent.width * 100
            property int focusIdInternal: focusId

            highlight: Rectangle {
                visible: focusIdInternal == thumbnails.focusedGrid
                width: currentItem.width
                height: currentItem.height
                color: "transparent"
                border.color: Style.nxpDarkGrey
                border.width: 2
                radius: 10
            }
            highlightMoveDuration: 0

            MouseArea {
                anchors.fill: parent
                onClicked: (mouse) => {
                    let posInGridView = Qt.point(mouse.x, mouse.y)
                    let posInContentItem = mapToItem(thumbnailsgrid.contentItem, posInGridView)
                    let item = thumbnailsgrid.itemAt(posInContentItem.x, posInContentItem.y)
                    let item_index = thumbnailsgrid.indexAt(posInContentItem.x, posInContentItem.y)
                    thumbnailsgrid.currentIndex = item_index
                    select(item)
                }
            }

            function select(item) {
                thumbnails.selectedFile = item.fileUrl
                thumbnails.focusedGrid = thumbnailsgrid.focusIdInternal
                thumbnails.fileSelected()
            }
        }
    }

    Column {
        anchors.fill: parent.fill
        spacing: 4
        Text { 
            text: "Test Pattern"
            font.bold: true
            font.pointSize: 12.0
            width: thumbnails.width
            height: thumbnails.height * 0.04
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Image {
            width: thumbnails.width * 0.9
            height: 1
            anchors.horizontalCenter : parent.horizontalCenter
            source: "qrc:/image/separator";
        }
        Loader {
            id: patternthumbnail
            property int focusId: 0
            width: thumbnails.width
            height: thumbnails.height * 0.28
            property var vmodel: testpatternmodel
            sourceComponent: thumbnailsview
        }
        Text { 
            text: "Video"
            font.bold: true
            font.pointSize: 12.0
            width: thumbnails.width
            height: thumbnails.height * 0.04
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Image {
            width: thumbnails.width * 0.9
            height: 1
            anchors.horizontalCenter : parent.horizontalCenter
            source: "qrc:/image/separator";
        }
        Loader {
            id: videothumbnail
            property int focusId: 1
            width: thumbnails.width
            height: thumbnails.height * 0.64 - 22
            property var vmodel: videomodel
            sourceComponent: thumbnailsview
        }
    }
}
