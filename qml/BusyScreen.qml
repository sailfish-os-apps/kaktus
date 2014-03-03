/*
  Copyright (C) 2014 Michal Kosciesza <michal@mkiol.net>

  This file is part of Kaktus.

  Kaktus is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Kaktus is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Kaktus.  If not, see <http://www.gnu.org/licenses/>.
*/

import QtQuick 2.0
import Sailfish.Silica 1.0



Rectangle {
    id: root

    property string text
    property bool cancelable: false
    property bool open: false
    property real progress: 0.0

    signal closeClicked

    anchors.fill: parent

    /*gradient: Gradient {
        GradientStop { position: 0.0; color: "transparent" }
        GradientStop { position: 0.95; color: Theme.highlightBackgroundColor}
    }*/

    color: Theme.rgba(Theme.backgroundColor, 1.0)
    enabled: opacity > 0.0

    opacity: root.open ? 1.0 : 0.0
    visible: opacity > 0.0
    Behavior on opacity { FadeAnimation {duration: 300} }

    function show(text, cancelable) {
        root.text = text;
        root.cancelable = cancelable;
        root.open = true;
    }

    function hide() {
        root.open = false;
    }

    BusyIndicator {
        id: busy
        anchors.centerIn: parent
        running: open
        size: BusyIndicatorSize.Large
    }

    onVisibleChanged: {
        if (!visible) {
            progress = 0;
        }
    }

    Item {
        height: Theme.itemSizeMedium * 1
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        Label {
            id: titleBar

            height: parent.height
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left; anchors.right: closeButton.right
            anchors.leftMargin: Theme.paddingMedium

            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            text: root.text
            verticalAlignment: Text.AlignVCenter
            color: Theme.primaryColor
        }

        IconButton {
            id: closeButton

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: Theme.paddingMedium

            icon.source: "image://theme/icon-m-close"
            onClicked: root.closeClicked()
            visible: root.cancelable
        }
    }


}