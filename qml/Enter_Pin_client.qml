import client
import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import MyDesigns


ColumnLayout
{
    id: ep

    spacing:0

    Connections {
        target: Book_Client
        function onTopay_changed() {
            if(Book_Client.topay)
            {
                payserver.addr_= Account.addr_bech32([0,0,0],Node_Conection.info().protocol.bech32Hrp)
                payserver.descr_=qsTr("Transfer at least "+ Book_Client.topay +" "+Node_Conection.info().baseToken.subunit+ " to:\n"+payserver.addr_);
                payserver.url_="firefly:v1/wallet/send?recipient="+payserver.addr_+"&amount="+Book_Client.topay
                payserver.visible=true;
            }
            else
            {
                payserver.visible=false;
            }
        }
        function onStateChanged() {
            if(Book_Client.state===Book_Client.Connected&&!Book_Client.topay)
            {
                ep.stack.pop();
            }
        }
    }
    MyPayPopUp
    {
        id:payserver
        property bool init:false
        descr_:""
        addr_: ""
        url_:""
        anchors.centerIn: Overlay.overlay
        visible:false
        background:Rectangle
        {
            color:"#0f171e"
            border.width: 1
            border.color: "white"
            radius:8
        }

    }


    MyLabel
    {
        text: qsTr("Set pin")
        Layout.alignment: Qt.AlignCenter
        font.pointSize: 16
    }
    TextInput {
        id:numbers_
        Layout.maximumHeight: 200
        Layout.fillHeight:  true
        Layout.maximumWidth: 300
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignCenter
        font.letterSpacing :20
        font.pointSize: 28
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color:"white"
        inputMask: "99999"
        text: "12345"
    }
    MyButton
    {
        Layout.maximumWidth: 150
        Layout.maximumHeight: 50
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter
        text: "Book"
        enabled: Book_Client.state===Book_Client.Connected
        onClicked:
        {
            Book_Client.code_str=numbers_.text;
            Day_model.get_new_bookings();

        }
    }

}


