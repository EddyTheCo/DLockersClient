pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick
import QtQuick.Layouts
import nodeConection
import account
import client
import booking_model
import MyDesigns

ApplicationWindow {
    visible: true


    Connections {
        target: Node_Conection
        function onStateChanged() {
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
            Book_Client.try_to_book(boo);
        }
    }
    Connections {
        target: Book_Client
        function onSent_book(boo) {
            Day_model.add_booking(boo,true);
        }
        function onGot_new_booking(boo) {
            Day_model.add_booking(boo,false);
        }
        function onRemoved_expired(boo) {
            Day_model.remove_sent_booking(boo);
        }
    }
    Popup {
        id:settings
        anchors.centerIn: Overlay.overlay
        visible:true
        closePolicy: Popup.NoAutoClose
        background:Rectangle
        {
            color:"#0f171e"
            border.width: 1
            border.color: "white"
            radius:8
        }
        ColumnLayout
        {
            anchors.fill:parent
            Node_Connections
            {
                id:nodeco
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: 350
                Layout.preferredHeight: 250
            }
            AccountQml
            {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: 500
                Layout.alignment: Qt.AlignCenter
                Layout.preferredHeight: 300
                Layout.preferredWidth:nodeco.width
            }
            MyTextField
            {
                id:serverid

                Layout.minimumHeight: 75
                Layout.preferredWidth:nodeco.width
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter
                desc:"Set the server id:"
                placeholderText:(Node_Conection.state)?Node_Conection.info().protocol.bech32Hrp+"1...(mandatory)":""
                tfield.onTextEdited: {
                    Book_Client.server_id=serverid.tfield.text
                }
            }
            MyButton
            {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: 50
                Layout.maximumWidth: 75
                Layout.preferredWidth: 50
                Layout.minimumHeight: 25
                Layout.minimumWidth:50
                Layout.alignment: Qt.AlignHCenter
                text:qsTr("Start")
                enabled: Node_Conection.state&&Book_Client.state

                onClicked:{
                    settings.close();
                }
            }
        }

    }


    StackView {
        id: stack_
        initialItem: initial
        anchors.fill: parent
    }

    Component
    {
        id:initial

        ColumnLayout
        {
            id:column
            spacing: 0
            Head
            {
                id:head
                Layout.maximumHeight: 300
                Layout.minimumHeight: 100
                Layout.fillHeight:  true
                Layout.minimumWidth: 300
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                butt.text:"Book"
                butt.enabled:Day_model.total_selected&&Book_Client.state

                butt.onClicked:
                {
                    var component = Qt.createComponent("Enter_Pin_client.qml");
                    if (component.status === Component.Ready) {
                        var next = component.createObject(column, {stack:stack_,headh:head.height});
                        if (next === null) {
                            console.log("Error creating object");
                        }
                    } else if (component.status === Component.Error) {
                        console.log("Error loading component:", component.errorString());
                    }
                    stack_.push(next)
                }
            }

            Rectangle
            {
                id:bott
                color:"#0f171e"
                Layout.minimumHeight: 500
                Layout.fillHeight:  true
                Layout.minimumWidth: 300
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Day_swipe_view {
                    id: dayview
                    clip:true
                    can_book:true
                    anchors.fill:parent
                }
            }

        }

    }


}

