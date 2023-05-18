import client
import QtQuick 2.0
import QtQuick.Layouts
import MyDesigns
import nodeConection
import account
import QtQuick.Controls


ColumnLayout
{
    id:head

    property alias  butt1:button1
    property alias  butt2:button2

    RowLayout
    {
        Layout.maximumHeight: 60
        Layout.minimumHeight: 50
        Layout.fillHeight:  true
        Layout.minimumWidth: 200
        Layout.maximumWidth: 300
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignCenter
        spacing: 10

        MyButton
        {
            id:button1
            Layout.alignment: Qt.AlignCenter
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 5
            Layout.minimumWidth: 100
        }
        BusyIndicator {
            running: Book_Client.state===Book_Client.Sending
        }
        MyButton
        {
            id:button2
            Layout.alignment: Qt.AlignCenter

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            Layout.margins: 5
        }
    }



    Rectangle
    {
        id:line
        Layout.alignment: Qt.AlignHCenter|Qt.AlignBottom
        Layout.maximumHeight: 2
        Layout.fillHeight: true
        Layout.fillWidth: true
        color: "#21be2b"
    }

}
