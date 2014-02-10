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


Page {
    id: root

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: tabModel

        MainMenu{}

        header: PageHeader {
            title: "Tabs"
        }

        /*delegate: ListItem {
            id: listItem
            //contentHeight: item.height + 2 * Theme.paddingMedium
            contentHeight: Math.max(image.height,label.height)+2*Theme.paddingMedium

            Image {
                id: image
                width: Theme.iconSizeSmall
                height: Theme.iconSizeSmall
                anchors.left: parent.left;
                anchors.leftMargin: Theme.paddingLarge; anchors.rightMargin: Theme.paddingLarge
                anchors.verticalCenter: parent.verticalCenter
                source: iconUrl
            }

            Label {
                id: label
                wrapMode: Text.AlignLeft
                anchors.left: image.right; anchors.right: parent.right;
                anchors.leftMargin: Theme.paddingLarge; anchors.rightMargin: Theme.paddingLarge
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: Theme.fontSizeMedium
                text: title
            }


            onClicked: {
                utils.setFeedModel(uid);
                pageStack.push(Qt.resolvedUrl("FeedPage.qml"),{"title": title});
            }

        }*/


        delegate: ListItem {
            id: listItem
            contentHeight: item.height + 2 * Theme.paddingMedium

            Column {
                id: item
                spacing: Theme.paddingSmall
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width

                Label {
                    wrapMode: Text.AlignLeft
                    anchors.left: parent.left; anchors.right: parent.right;
                    anchors.leftMargin: Theme.paddingLarge; anchors.rightMargin: Theme.paddingLarge
                    font.pixelSize: Theme.fontSizeMedium
                    text: title
                }
            }

            onClicked: {
                utils.setFeedModel(uid);
                pageStack.push(Qt.resolvedUrl("FeedPage.qml"),{"title": title});
            }
        }


        ViewPlaceholder {
            enabled: listView.count == 0
            //enabled: true
            text: qsTr("No tabs")
        }

        VerticalScrollDecorator {
            flickable: listView
        }

    }

}