import client
import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import MyDesigns


ColumnLayout
{
    id: ep
    required property  StackView stack
    required property int headh
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
    Head
    {
        id:head
        Layout.preferredHeight:ep.headh
        Layout.maximumHeight: ep.headh
        Layout.minimumHeight: 100
        Layout.fillHeight:  true
        Layout.minimumWidth: 300
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        butt.text:"Back"
        butt.onClicked:ep.stack.pop()
    }


    Rectangle
    {
        id:center_
        color:"#0f171e"
        Layout.fillHeight:  true
        Layout.minimumWidth: 300
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop


        ColumnLayout
        {
            spacing:20
            anchors.fill: parent
            MyPinBox
            {
                id:pin_box_

                Layout.maximumHeight: 200
                Layout.fillHeight:  true
                Layout.maximumWidth: 300
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignCenter

                description:qsTr("Set pin:")
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
                    Book_Client.code_str=pin_box_.pin.text;
                    Day_model.get_new_bookings();
                }
            }

        }

    }

}
