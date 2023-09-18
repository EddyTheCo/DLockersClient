pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick
import QtQuick.Layouts
import nodeConection
import account
import client
import booking_model
import MyDesigns
import QtQrDec
import QtQrGen

ApplicationWindow {
    visible: true
    id:window
    FontLoader {
        id: webFont
        source: "qrc:/esterVtech.com/imports/client/qml/fonts/DeliciousHandrawn-Regular.ttf"
    }
    Component.onCompleted:
    {
        if(LocalConf.nodeaddr) Node_Conection.nodeaddr=LocalConf.nodeaddr;
        if(LocalConf.jwt) Node_Conection.jwt=LocalConf.jwt;

        CustomStyle.h1=Qt.font({
                                   family: webFont.font.family,
                                   weight: webFont.font.weight,
                                   pixelSize: 28
                               });
        CustomStyle.h2=Qt.font({
                                   family: webFont.font.family,
                                   weight: webFont.font.weight,
                                   pixelSize: 28
                               });
    }


    background: Rectangle
    {
        color:CustomStyle.backColor1
    }
    Notification
    {
        id:noti
        width:300
        height:100
        x:(window.width-width)*0.5
        y: window.height*(1-0.05)-height
    }

    Connections {
        target: Node_Conection
        function onStateChanged() {
            if(Node_Conection.state==Node_Conection.Connected)
            {
                noti.show({"message":"Conected to "+ Node_Conection.nodeaddr });
            }
            Book_Client.restart();
        }
    }
    Connections {
        target: Account
        function onSeedChanged() {
            Book_Client.restart();
        }
    }
    Connections {
        target: Day_model
        function onHasnewbooks(boo) {
            Book_Client.send_booking(boo);
        }
    }
    Connections {
        target: Book_Client
        function onSent_book(books,id) {
            Day_model.add_booking(books,id);
        }
        function onGot_new_booking(books) {
            Day_model.add_booking(books);
        }
        function onRemoved_expired(outid) {
            Day_model.remove_sent_booking(outid);
        }
        function onNotEnought(amount) {
            noti.show({"message":"Not enough funds\n "+ "lack of "+ amount.largeValue.value + " "+ amount.largeValue.unit });
        }
    }
    Drawer {
        id:settings
        width:300

        height:parent.height
        focus:true
        modal:true

        background: Rectangle
        {
            color:CustomStyle.backColor1
        }
        ColumnLayout
        {
            anchors.fill:parent
            Rectangle
            {
                color:CustomStyle.backColor2
                radius:Math.min(width,height)*0.05
                Layout.minimumWidth: 100
                Layout.maximumHeight: (Node_Conection.state)?150:100
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: (Node_Conection.state)? 100:50
                Layout.alignment: Qt.AlignHCenter|Qt.AlignTop
                Layout.margins: 20
                border.color:CustomStyle.midColor1
                border.width: 1
                ColumnLayout
                {
                    anchors.fill:parent
                    QrLabel
                    {
                        visible:Node_Conection.state
                        description:qsTr("<b>Client id</b>")
                        address:Book_Client.clientId
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 30
                        Layout.margins: 5
                    }
                    Text
                    {
                        font:(Node_Conection.state)?CustomStyle.h3:CustomStyle.h2
                        text:(Node_Conection.state)?qsTr("Available Balance: "):qsTr("Waiting for node")
                        horizontalAlignment:Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: CustomStyle.frontColor1
                        fontSizeMode:Text.Fit
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                        Layout.fillHeight: true
                    }
                    AmountText
                    {
                        visible:Node_Conection.state
                        font:CustomStyle.h2
                        jsob:Book_Client.funds
                        horizontalAlignment:Text.AlignHCenter
                        fontSizeMode:Text.Fit
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                        Layout.fillHeight: true
                    }

                }

            }
            MyButton
            {
                id:serverid

                Layout.alignment: Qt.AlignCenter
                text: qsTr("Set the server")
                Layout.margins: 5
                onClicked:
                {
                    popup_.setServer=true;
                    popup_.open();
                }
            }
            Node_Connections
            {
                id:conn_
                Layout.fillWidth: true
                Layout.minimumHeight: 30
            }
            AccountQml
            {
                id:acc_
                Layout.fillWidth: true
                Layout.minimumHeight: 30
            }


        }

    }
    ColumnLayout
    {
        id:column
        spacing: 0
        anchors.fill:parent
        Head
        {
            id:head

            Layout.maximumHeight: 200
            Layout.minimumHeight: 100
            Layout.minimumWidth: 300
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop
            butt1.text:qsTr("Book")

            butt1.enabled:Day_model.total_selected&&Book_Client.state===Book_Client.Connected

            butt1.onClicked:
            {
                Day_model.get_new_bookings();

                Book_Client.state=Book_Client.Sending
            }

            butt2.text:qsTr("Open")

            butt2.enabled:Book_Client.state===Book_Client.Connected

            butt2.onClicked:
            {

                popup_.setServer=false;
                popup_.open();

            }
            QrTextArea
            {
                id:popup_
                property bool setServer:true;
                visible:false
                closePolicy: Popup.CloseOnPressOutside
                width:300
                height:425
                anchors.centerIn: Overlay.overlay

                description: (popup_.setServer)?qsTr("Set the server address:"):qsTr("Present the nft to:")
                placeholder: (Node_Conection.state)?Node_Conection.info().protocol.bech32Hrp+"1":""
                onClicked: (data) => {
                    if(popup_.setServer)
                    {
                        Book_Client.server_id=data;
                    }
                    else
                    {
                        Book_Client.presentNft(data);
                    }
                }
            }
        }

        Day_swipe_view {
            id: dayview
            clip:true
            can_book:true
            Layout.fillWidth: true
            Layout.fillHeight:  true
            Layout.minimumWidth: 300
            Layout.maximumWidth: 700
            Layout.alignment: Qt.AlignTop|Qt.AlignHCenter
        }

    }

    MySettButton
    {
        id:seetbutt
        width: 40
        x:settings.width*settings.position
        y:(window.height-height)*0.5
        height:width
        onClicked:
        {
            settings.open()
        }
        animate: settings.position>0.1

    }

}



