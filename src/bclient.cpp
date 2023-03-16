#include"bclient.hpp"
#include <QCryptographicHash>
#include <QDebug>
#include<QJsonDocument>
#include<QTimer>
using namespace qiota::qblocks;

using namespace qiota;
using namespace qcrypto;

Book_Client::ConState Book_Client::state_=Book_Client::Disconnected;

Book_Client::Book_Client(QObject *parent):QObject(parent),topay_(0)
{
    QObject::connect(this,&Book_Client::server_idChanged,this,&Book_Client::monitor_state);
}

void Book_Client::check_state_output(const Node_output& node_output_)
{
    if(node_output_.output()->type_m==qblocks::Output::Basic_typ)
    {
        const auto basic_output_=std::dynamic_pointer_cast<qblocks::Basic_Output>(node_output_.output());
        const auto sendfea=basic_output_->get_feature_(qblocks::Feature::Sender_typ);
        if(sendfea&&std::dynamic_pointer_cast<qblocks::Sender_Feature>(sendfea)->sender()->addr()==server_id_)
        {

            const auto tagfeau=basic_output_->get_feature_(qblocks::Feature::Tag_typ);
            if(tagfeau&&std::dynamic_pointer_cast<qblocks::Tag_Feature>(tagfeau)->tag()==fl_array<quint8>("state"))
            {

                const auto metfeau=basic_output_->get_feature_(qblocks::Feature::Metadata_typ);
                if(metfeau)
                {

                    auto metadata_feature=std::dynamic_pointer_cast<qblocks::Metadata_Feature>(metfeau);
                    auto metadata=metadata_feature->data();
                    auto server_data=Booking::deserialize_state(metadata);

                    if(std::get<2>(server_data)!=QByteArray(32,0))
                    {
                        price_per_hour_=std::get<1>(server_data);
                        paytoaddrhash_=std::get<2>(server_data);
                        for(const auto& new_booking:std::get<0>(server_data))
                        {

                            emit got_new_booking(new_booking);
                        }
                        set_state(Connected);
                    }

                }
            }
        }
    }


}
void Book_Client::monitor_state(void)
{

    if(Node_Conection::state()==Node_Conection::Connected)
    {
        if(!server_id_.isNull())
        {
            if(reciever)reciever->deleteLater();
            reciever=new QObject(this);

            auto info=Node_Conection::rest_client->get_api_core_v2_info();
            QObject::connect(info,&Node_info::finished,reciever,[=]( ){

                auto node_outputs_=new Node_outputs();
                Node_Conection::rest_client->get_basic_outputs(node_outputs_,"address="+get_server_id()+
                                                               "&hasStorageDepositReturn=false&hasTimelock=false&hasExpiration=false&sender="
                                                               +get_server_id()+"&tag="+fl_array<quint8>("state").toHexString());
                QObject::connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){
                    if(node_outputs_->outs_.size())
                    {
                        check_state_output(node_outputs_->outs_.front());
                    }
                    auto resp=Node_Conection::mqtt_client->
                            get_outputs_unlock_condition_address("address/"+get_server_id());
                    QObject::connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
                        const auto node_outputs_2=Node_output(data);
                        check_state_output(node_outputs_2);
                    });
                    info->deleteLater();
                    node_outputs_->deleteLater();
                });


                auto resp=Node_Conection::mqtt_client->
                        get_outputs_unlock_condition_address("address/"+Account::addr_bech32({0,0,0},info->bech32Hrp));
                QObject::connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
                    set_to_pay(0);
                });
            });
        }
    }
}


void Book_Client::send_booking(Booking book,std::vector<Node_output> &outputs,Node_info* info)
{
    auto payment_bundle=Account::get_addr({0,0,0});

    book.set_code_str(code_str_);

    auto metadata_=Booking::create_new_bookings_metadata(book);
    auto metFea=std::shared_ptr<qblocks::Feature>(new Metadata_Feature(metadata_));

    auto price=book.calculate_price(price_per_hour_);

    auto retAddr=std::shared_ptr<Address>(new Ed25519_Address(payment_bundle.get_hash()));
    auto sendFea=std::shared_ptr<qblocks::Feature>(new Sender_Feature(retAddr));

    auto var_storageaddrUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(
                new Storage_Deposit_Return_Unlock_Condition(retAddr,0));

    auto until_when=QDateTime::currentDateTime().addSecs(5*60).toSecsSinceEpoch();
    QTimer::singleShot(450000, this, &Book_Client::check_if_expired);
    auto expirationUnlock=std::shared_ptr<qblocks::Unlock_Condition>
            (new Expiration_Unlock_Condition(until_when,retAddr));
    auto pay_to_address=std::shared_ptr<Address>(new Ed25519_Address(paytoaddrhash_));


    auto pay_toaddrUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(new Address_Unlock_Condition(pay_to_address));

    auto min_deposit_pay_to= Basic_Output(0,{pay_toaddrUnlcon,var_storageaddrUnlcon,expirationUnlock},{sendFea,metFea},{})
            .min_deposit_of_output(info->vByteFactorKey,info->vByteFactorData,info->vByteCost);

    c_array Inputs_Commitments;
    quint64 amount=0;
    std::vector<std::shared_ptr<qblocks::Output>> ret_outputs;
    std::vector<std::shared_ptr<qblocks::Input>> inputs;

    payment_bundle.consume_outputs(outputs,price+min_deposit_pay_to,Inputs_Commitments,amount,
                                   ret_outputs,inputs);

    if(amount>=price+min_deposit_pay_to)
    {

        auto storageaddrUnlcon=std::shared_ptr<qblocks::Unlock_Condition>(
                    new Storage_Deposit_Return_Unlock_Condition(retAddr,amount-price));
        auto Inputs_Commitment=c_array(QCryptographicHash::hash(Inputs_Commitments, QCryptographicHash::Blake2b_256));
        auto BaOut= std::shared_ptr<qblocks::Output>
                (new Basic_Output(amount,{pay_toaddrUnlcon,storageaddrUnlcon,expirationUnlock},{sendFea,metFea},{}));

        ret_outputs.push_back(BaOut);
        auto essence=std::shared_ptr<qblocks::Essence>
                (new Transaction_Essence(info->network_id_,inputs,Inputs_Commitment,ret_outputs,nullptr));

        c_array serializedEssence;
        serializedEssence.from_object<Essence>(*essence);

        auto essence_hash=QCryptographicHash::hash(serializedEssence, QCryptographicHash::Blake2b_256);
        std::vector<std::shared_ptr<qblocks::Unlock>> unlocks;
        payment_bundle.create_unlocks<qblocks::Reference_Unlock>(essence_hash,unlocks);

        auto trpay=std::shared_ptr<qblocks::Payload>(new Transaction_Payload(essence,unlocks));

        auto block_=Block(trpay);
        auto resp=Node_Conection::mqtt_client->
                get_outputs_unlock_condition_address("address/"+qencoding::qbech32::Iota::encode(info->bech32Hrp,pay_to_address->addr()));
        QObject::connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
            const auto node_output_=Node_output(data);
            if(node_output_.output()->type_m==qblocks::Output::Basic_typ)
            {
                const auto basic_output_=std::dynamic_pointer_cast<qblocks::Basic_Output>(node_output_.output());
                const auto sendfea=basic_output_->get_feature_(qblocks::Feature::Sender_typ);
                if(sendfea&&std::dynamic_pointer_cast<qblocks::Sender_Feature>(sendfea)->sender()->addr()==
                        Account::get_addr({0,0,0}).get_address<qblocks::Address::Ed25519_typ>())
                {
                    set_state(Connected);
                    resp->deleteLater();
                }
            }

        });
        QTimer::singleShot(60000,resp,[=](){this->set_state(Connected); resp->deleteLater();});
        set_state(Sending);
        Node_Conection::rest_client->send_block(block_);
        books_.push_back(book);
        emit sent_book(book);
        set_to_pay(0);
    }
    else
    {
        set_to_pay(topay_+price+min_deposit_pay_to-amount);
    }
}
void Book_Client::try_to_book(const std::vector<Booking>& books)
{

    check_if_expired();
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,this,[=]( ){

        auto node_outputs_expired_=new Node_outputs();
        Node_Conection::rest_client->get_basic_outputs(node_outputs_expired_,
                                                       "expirationReturnAddress="+Account::addr_bech32({0,0,0},info->bech32Hrp));

        connect(node_outputs_expired_,&Node_outputs::finished,this,[=]( ){
            auto node_outputs_=new Node_outputs();
            Node_Conection::rest_client->get_basic_outputs(node_outputs_,"address="+Account::addr_bech32({0,0,0},info->bech32Hrp));

            QObject::connect(node_outputs_,&Node_outputs::finished,this,[=]( ){

                std::vector<Node_output> outs_=node_outputs_expired_->outs_;
                std::move(node_outputs_->outs_.begin(),node_outputs_->outs_.end(), std::back_inserter(outs_));
                set_to_pay(0);
                for(const auto& book:books)
                {
                    send_booking(book,outs_,info);
                }
                info->deleteLater();
                node_outputs_->deleteLater();
                node_outputs_expired_->deleteLater();
            });

        });

    });

}

void Book_Client::check_if_expired(void)
{
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){

        auto node_outputs_=new Node_outputs();
        Node_Conection::rest_client->get_basic_outputs(node_outputs_,
                                                       "expirationReturnAddress="+Account::addr_bech32({0,0,0},info->bech32Hrp)+
                                                       "&sender="+Account::addr_bech32({0,0,0},info->bech32Hrp));

        connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){

            for(const auto& out: node_outputs_->outs_)
            {
                const auto basic_output_=std::dynamic_pointer_cast<qblocks::Basic_Output>(out.output());
                const auto metfeature=basic_output_->get_feature_(qblocks::Feature::Metadata_typ);

                if(metfeature)
                {
                    auto metadata_feature=std::dynamic_pointer_cast<qblocks::Metadata_Feature>(metfeature);
                    auto metadata=metadata_feature->data();
                    auto expired_book=Booking::get_new_booking_from_metadata(metadata);
                    auto it=std::find_if(books_.begin(),books_.end(),[=](const auto& book_pair){
                        return (book_pair.start()==expired_book.start()&&book_pair.finish()==expired_book.finish());
                    });
                    if(it!=books_.end())
                    {
                        emit removed_expired(expired_book);
                        books_.erase(it);
                    }
                }

            }
            info->deleteLater();
            node_outputs_->deleteLater();
        });

    });


}

QString Book_Client::get_server_id(void)const
{
    if(server_id_.isNull())
        return QString("");
    return qencoding::qbech32::Iota::encode(Node_Conection::rest_client->info()["protocol"].toObject()["bech32Hrp"].toString(),server_id_);
}
void Book_Client::set_server_id(QString server_id_m)
{
    const auto addr_pair=qencoding::qbech32::Iota::decode(server_id_m);
    if(addr_pair.second.size()&&addr_pair.second!=server_id_)
    {
        server_id_=addr_pair.second;
        emit server_idChanged();
    }
}
