import client
import QtQuick 2.0
import QtQuick.Layouts
import MyDesigns
import nodeConection
import account

Rectangle
{
    id:head
    color:"#0f171e"
    property alias  butt:button

    ColumnLayout
    {
        anchors.fill:head
        MyButton
        {
            id:button
            Layout.maximumHeight: 75
            Layout.minimumHeight: 50
            Layout.fillHeight:  true
            Layout.minimumWidth: 35
            Layout.maximumWidth: 150
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
        }
    }
    Rectangle
    {
        id:line
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height:2
        color: "#21be2b"
    }
}
