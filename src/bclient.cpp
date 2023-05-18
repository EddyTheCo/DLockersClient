#include"bclient.hpp"
#include <QCryptographicHash>
#include <QDebug>
#include<QJsonDocument>
#include<QTimer>
using namespace qiota::qblocks;

using namespace qiota;
using namespace qcrypto;

Book_Client::ConState Book_Client::state_=Book_Client::Disconnected;

Book_Client::Book_Client(QObject *parent):QObject(parent),reciever(nullptr),inited(false)
{
    QObject::connect(this,&Book_Client::server_idChanged,this,&Book_Client::monitor_state);
}

void Book_Client::check_state_output(const Node_output& node_output_)
{
    if(node_output_.output()->type()==qblocks::Output::Basic_typ)
    {
        const auto output_=node_output_.output();
        const auto sendfea=output_->get_feature_(qblocks::Feature::Sender_typ);
        if(sendfea&&std::static_pointer_cast<const Sender_Feature>(sendfea)->sender()->addr()==server_id_)
        {

            const auto tagfeau=output_->get_feature_(Feature::Tag_typ);
            if(tagfeau&&std::static_pointer_cast<const Tag_Feature>(tagfeau)->tag()==fl_array<quint8>("state"))
            {

                const auto metfeau=output_->get_feature_(Feature::Metadata_typ);
                if(metfeau)
                {

                    auto metadata_feature=std::static_pointer_cast<const Metadata_Feature>(metfeau);
                    auto metadata=metadata_feature->data();

                    deserialize_state(metadata);

                }
            }
        }
    }


}
void Book_Client::deserialize_state(const QByteArray &state)
{
    const auto var=QJsonDocument::fromJson(state).object();
    if(var["price_per_hour"].isUndefined())return;
    price_per_hour_=var["price_per_hour"].toString().toULongLong();
    if(var["pay_to"].isUndefined())return;
    paytoaddr_=c_array::fromHexString(var["pay_to"].toString());

    if(!var["bookings"].isUndefined()&&var["bookings"].isArray())
    {
        const auto bookarray=var["bookings"].toArray();
        if(!inited)
        {
            set_state(Connected);
            inited=true;
        }

        emit got_new_booking(bookarray);
    }

}
void Book_Client::setFunds(quint64 funds_m){

    if(funds_!=funds_m||funds_m==0)
    {
        funds_=funds_m;
        auto info=Node_Conection::rest_client->get_api_core_v2_info();
        QObject::connect(info,&Node_info::finished,reciever,[=]( ){
            funds_json=info->amount_json(funds_);
            emit fundsChanged();
            info->deleteLater();
        });
    }
}
void Book_Client::checkFunds(std::vector<qiota::Node_output>  outs)
{
    quint64 total=0;
    for(const auto& v:outs)
    {
        std::vector<Node_output> var{v};
        auto bundle= Account::get_addr({0,0,0});
        bundle.consume_outputs(var);
        if(bundle.amount)
        {
            total+=bundle.amount;
            total_funds.insert(v.metadata().outputid_.toHexString(),bundle.amount);
            auto resp=Node_Conection::mqtt_client->get_outputs_outputId(v.metadata().outputid_.toHexString());
            connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
                const auto node_output=Node_output(data);
                if(node_output.metadata().is_spent_)
                {
                    auto it=total_funds.constFind(node_output.metadata().outputid_.toHexString());
                    if(it!=total_funds.cend())
                    {
                        setFunds(funds_-it.value());
                        total_funds.erase(it);
                    }
                    resp->deleteLater();
                }

            });
        }
        if(bundle.to_expire.size())
        {
            const auto unixtime=bundle.to_expire.front();
            const auto triger=(unixtime-QDateTime::currentDateTime().toSecsSinceEpoch())*1000;
            QTimer::singleShot(triger,reciever,[=](){
                auto it=total_funds.constFind(v.metadata().outputid_.toHexString());
                if(it!=total_funds.cend())
                {
                    setFunds(funds_-it.value());
                    total_funds.erase(it);
                }
            });
        }
        if(bundle.to_unlock.size())
        {
            const auto unixtime=bundle.to_unlock.front();
            const auto triger=(unixtime-QDateTime::currentDateTime().toSecsSinceEpoch())*1000;
            QTimer::singleShot(triger+5000,reciever,[=](){
                auto resp=Node_Conection::mqtt_client->get_outputs_outputId(v.metadata().outputid_.toHexString());
                connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
                    const auto node_output=Node_output(data);
                    checkFunds({node_output});
                    resp->deleteLater();
                });
            });
        }

    }

    setFunds(funds_+total);
}
void Book_Client::monitor_state(void)
{

    if(Node_Conection::state()==Node_Conection::Connected)
    {
        if(reciever)reciever->deleteLater();
        reciever=new QObject(this);
        setFunds(0);
        total_funds.clear();
        auto info=Node_Conection::rest_client->get_api_core_v2_info();
        QObject::connect(info,&Node_info::finished,reciever,[=]( ){
            setClientId(Account::addr_bech32({0,0,0},info->bech32Hrp));
            if(!server_id_.isNull())
            {


                auto node_outputs_=new Node_outputs();
                const auto serverAddress=qencoding::qbech32::Iota::encode(info->bech32Hrp,server_id_);
                QObject::connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){
                    if(node_outputs_->outs_.size())
                    {
                        check_state_output(node_outputs_->outs_.front());
                    }
                    auto resp=Node_Conection::mqtt_client->
                            get_outputs_unlock_condition_address("address/"+serverAddress);
                    QObject::connect(resp,&ResponseMqtt::returned,reciever,[=](QJsonValue data){
                        const auto node_outputs_2=Node_output(data);
                        check_state_output(node_outputs_2);
                    });

                    node_outputs_->deleteLater();
                });

                Node_Conection::rest_client->get_outputs<Output::Basic_typ>
                        (node_outputs_,"address="+serverAddress
                         +"&hasStorageDepositReturn=false&hasTimelock=false&hasExpiration=false&sender="
                         +get_server_id()+"&tag="+fl_array<quint8>("state").toHexString());

            }
            auto resp2=Node_Conection::mqtt_client->
                    get_outputs_unlock_condition_address("address/"+Account::addr_bech32({0,0,0},info->bech32Hrp));
            connect(resp2,&ResponseMqtt::returned,reciever,[=](QJsonValue data)
            {
                checkFunds({Node_output(data)});

            });

            auto node_outputs=new Node_outputs();
            connect(node_outputs,&Node_outputs::finished,reciever,[=]( ){
                auto node_outputs2=new Node_outputs();
                connect(node_outputs2,&Node_outputs::finished,reciever,[=]( ){
                    std::move(node_outputs2->outs_.begin(),node_outputs2->outs_.end(), std::back_inserter(node_outputs->outs_));
                    checkFunds(node_outputs->outs_);
                    node_outputs->deleteLater();
                    node_outputs2->deleteLater();
                });

                Node_Conection::rest_client->get_outputs<Output::Basic_typ>(node_outputs2,"expirationReturnAddress="+
                                                                            Account::addr_bech32({0,0,0},info->bech32Hrp));
                 info->deleteLater();
            });
            Node_Conection::rest_client->get_outputs<Output::Basic_typ>(node_outputs,"address="+
                                                                        Account::addr_bech32({0,0,0},info->bech32Hrp));

        });

    }
}


void Book_Client::send_booking(const QJsonArray books)
{
    QTimer::singleShot(15000,this,[=](){this->set_state(Connected);});
    check_if_expired();

    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,this,[=]( ){


        auto node_outputs_expired_=new Node_outputs();

        connect(node_outputs_expired_,&Node_outputs::finished,this,[=]( ){


            auto node_outputs_=new Node_outputs();
            QObject::connect(node_outputs_,&Node_outputs::finished,this,[=]( ){


                std::vector<Node_output> outs_=node_outputs_expired_->outs_;
                std::move(node_outputs_->outs_.begin(),node_outputs_->outs_.end(), std::back_inserter(outs_));
                auto bundle=Account::get_addr({0,0,0});
                bundle.consume_outputs(outs_);



                auto metadata_=Booking::create_new_bookings_metadata(books);

                auto metFea=Feature::Metadata(metadata_);

                auto retAddr=bundle.get_address();

                auto sendFea=Feature::Sender(retAddr);


                auto until_when=QDateTime::currentDateTime().addSecs(60).toSecsSinceEpoch();

                QTimer::singleShot(70000, this, &Book_Client::check_if_expired);

                auto expirationUnlock=Unlock_Condition::Expiration(until_when,retAddr);

                auto pay_to_address=Address::from_array(paytoaddr_);

                auto addrUnlcon=Unlock_Condition::Address(pay_to_address);
                auto mineUnlock=Unlock_Condition::Address(retAddr);

                auto price=Booking::calculate_price(books,price_per_hour_);

                auto minBa=Client::get_deposit(Output::Basic(0,{addrUnlcon}),info);

                auto storUnlo=Unlock_Condition::Storage_Deposit_Return(retAddr,minBa);

                auto BaOut=Output::Basic(0,{addrUnlcon,storUnlo,expirationUnlock},{},{sendFea,metFea});
                auto mind=Client::get_deposit(BaOut,info);


                BaOut->amount_=minBa+price;
                if(BaOut->amount_<mind)
                {
                    storUnlo=Unlock_Condition::Storage_Deposit_Return(retAddr,mind-price);
                    BaOut=Output::Basic(mind,{addrUnlcon,storUnlo,expirationUnlock},{},{sendFea,metFea});
                }

                quint16 index=0;

                if(bundle.amount>=BaOut->amount_)
                {
                    pvector<const Output> theoutputs;
                    if(bundle.amount>BaOut->amount_)
                    {
                        auto BaOut2=Output::Basic(bundle.amount-BaOut->amount_,
                                                  {mineUnlock},{},{});
                        if(Client::get_deposit(BaOut2,info)<=BaOut2->amount_)
                        {
                            theoutputs.push_back(BaOut2);
                            index=1;
                        }
                        else
                        {
                            BaOut->amount_=bundle.amount;  //here is giving more money than expected
                        }

                    }
                    theoutputs.push_back(BaOut);
                    theoutputs.insert(theoutputs.end(), bundle.ret_outputs.begin(), bundle.ret_outputs.end());


                    auto Inputs_Commitment=Block::get_inputs_Commitment(bundle.Inputs_hash);
                    auto essence=Essence::Transaction(info->network_id_,bundle.inputs,Inputs_Commitment,theoutputs);
                    bundle.create_unlocks(essence->get_hash());

                    auto trpay=Payload::Transaction(essence,bundle.unlocks);

                    auto resp=Node_Conection::mqtt_client->get_subscription("transactions/"+trpay->get_id().toHexString() +"/included-block");
                    connect(resp,&ResponseMqtt::returned,this,[=](auto var){
                        set_state(Connected);
                        resp->deleteLater();
                    });
                    auto block_=Block(trpay);
                    Node_Conection::rest_client->send_block(block_);


                    auto var=trpay->get_id();
                    var.append(index);

                    emit sent_book(books,var.toHexString());
                }
                else
                {
                    set_state(Connected);
                    emit notEnought(info->amount_json(BaOut->amount_-bundle.amount));
                }


                info->deleteLater();
                node_outputs_->deleteLater();
                node_outputs_expired_->deleteLater();
            });

            Node_Conection::rest_client->get_outputs<Output::Basic_typ>(node_outputs_,"address="+Account::addr_bech32({0,0,0},info->bech32Hrp));

        });
        Node_Conection::rest_client->get_outputs<Output::Basic_typ>(node_outputs_expired_,
                                                                    "expirationReturnAddress="+Account::addr_bech32({0,0,0},info->bech32Hrp));

    });


}
void Book_Client::presentNft(const QString address)
{
    const auto addr_pair=qencoding::qbech32::Iota::decode(address);

    if(addr_pair.second.size())
    {
        set_state(Sending);
        QTimer::singleShot(15000,this,[=](){this->set_state(Connected);});
        auto info=Node_Conection::rest_client->get_api_core_v2_info();
        QObject::connect(info,&Node_info::finished,reciever,[=]( ){

            auto node_outputs_=new Node_outputs();

            auto issuer=qencoding::qbech32::Iota::encode(info->bech32Hrp,server_id_);

            connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){

                auto node_outputs_expired_=new Node_outputs();
                connect(node_outputs_expired_,&Node_outputs::finished,this,[=]( ){

                    std::move(node_outputs_expired_->outs_.begin(),node_outputs_expired_->outs_.end(), std::back_inserter(node_outputs_->outs_));

                    const auto now=QDateTime::currentDateTime();

                    for(const auto&v:node_outputs_->outs_)
                    {
                        const auto output_=v.output();
                        const auto metfeau=output_->get_immutable_feature_(Feature::Metadata_typ);

                        if(metfeau)
                        {
                            auto metadata_feature=std::static_pointer_cast<const Metadata_Feature>(metfeau);
                            auto metadata=metadata_feature->data();

                            const auto var=QJsonDocument::fromJson(metadata).object();

                            if(!var["bookings"].isUndefined()&&var["bookings"].isArray())
                            {
                                const auto bookarray=var["bookings"].toArray();
                                const auto vec=Booking::from_Array(bookarray);

                                for(const auto& f:vec)
                                {

                                    if(f["start"].toInteger()<=now.toSecsSinceEpoch()&&f["finish"].toInteger()>now.toSecsSinceEpoch())
                                    {
                                        sendNft(v,addr_pair.second);
                                        info->deleteLater();
                                        node_outputs_->deleteLater();
                                        node_outputs_expired_->deleteLater();
                                        return;
                                    }
                                }
                            }

                        }
                    }


                    info->deleteLater();
                    node_outputs_->deleteLater();
                    node_outputs_expired_->deleteLater();
                });
                Node_Conection::rest_client->
                        get_outputs<Output::NFT_typ>(node_outputs_expired_,
                                                     "expirationReturnAddress="+Account::addr_bech32({0,0,0},info->bech32Hrp)+
                                                     "&issuer="+issuer);

            });

            Node_Conection::rest_client->
                    get_outputs<Output::NFT_typ>(node_outputs_,
                                                 "address="+Account::addr_bech32({0,0,0},info->bech32Hrp)+
                                                 "&issuer="+issuer);
        });
    }

}
void Book_Client::sendNft(Node_output out,c_array address)
{

    const auto eddAddr=Address::from_array(address);
    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){

        auto node_outputs_expired_=new Node_outputs();

        connect(node_outputs_expired_,&Node_outputs::finished,this,[=]( ){

            auto node_outputs_=new Node_outputs();
            QObject::connect(node_outputs_,&Node_outputs::finished,this,[=]( ){
                std::vector<Node_output> outs_={out};
                auto bundle=Account::get_addr({0,0,0});
                bundle.consume_outputs(outs_);

                if(bundle.nft_outputs.size())
                {
                    auto nftOut=bundle.nft_outputs.front();

                    const auto addUnlock=Unlock_Condition::Address(eddAddr);

                    auto retAddress=bundle.get_address();
                    const auto retUnlock=Unlock_Condition::Address(retAddress);
                    const auto expirunlock=Unlock_Condition::Expiration(1000,retAddress);
                    nftOut->unlock_conditions_={addUnlock,expirunlock};
                    nftOut->amount_=Client::get_deposit(nftOut,info);

                    std::move(node_outputs_expired_->outs_.begin(),node_outputs_expired_->outs_.end(), std::back_inserter(outs_));

                    std::move(node_outputs_->outs_.begin(),node_outputs_->outs_.end(), std::back_inserter(outs_));

                    bundle.consume_outputs(outs_,nftOut->amount_);

                    if(bundle.amount>=nftOut->amount_)
                    {
                        auto BaOut=Output::Basic(0,{retUnlock},bundle.get_tokens());
                        if(bundle.amount>nftOut->amount_)
                        {
                            auto min_deposit=Client::get_deposit(BaOut,info);
                            if(min_deposit>bundle.amount-nftOut->amount_)
                            {
                                bundle.consume_outputs(outs_
                                                       ,min_deposit-(bundle.amount-nftOut->amount_));
                                BaOut->native_tokens_=bundle.get_tokens();
                                min_deposit=Client::get_deposit(BaOut,info);
                                if(bundle.amount>=min_deposit+nftOut->amount_)BaOut->amount_=bundle.amount-nftOut->amount_;
                            }
                            else
                            {
                                BaOut->amount_=bundle.amount-nftOut->amount_;
                            }

                        }
                        pvector<const Output> the_outputs_{nftOut};
                        if(BaOut->amount_)the_outputs_.push_back(BaOut);
                        the_outputs_.insert(the_outputs_.end(), bundle.ret_outputs.begin(), bundle.ret_outputs.end());

                        auto Inputs_Commitment=Block::get_inputs_Commitment(bundle.Inputs_hash);

                        auto essence=Essence::Transaction(info->network_id_,bundle.inputs,Inputs_Commitment,the_outputs_);

                        bundle.create_unlocks(essence->get_hash());

                        auto trpay=Payload::Transaction(essence,bundle.unlocks);
                        auto resp=Node_Conection::mqtt_client->get_subscription("transactions/"+trpay->get_id().toHexString() +"/included-block");
                        connect(resp,&ResponseMqtt::returned,this,[=](auto var){
                            set_state(Connected);
                            resp->deleteLater();
                        });
                        auto block_=Block(trpay);
                        Node_Conection::rest_client->send_block(block_);

                    }
                    else
                    {
                        emit notEnought(info->amount_json(nftOut->amount_-bundle.amount));
                    }

                }

                info->deleteLater();
                node_outputs_->deleteLater();
                node_outputs_expired_->deleteLater();
            });
            Node_Conection::rest_client->get_outputs<Output::Basic_typ>(node_outputs_,"address="+Account::addr_bech32({0,0,0},info->bech32Hrp));

        });
        Node_Conection::rest_client->get_outputs<Output::Basic_typ>(node_outputs_expired_,
                                                                    "expirationReturnAddress="+Account::addr_bech32({0,0,0},info->bech32Hrp));

    });
}
void Book_Client::check_if_expired(void)
{

    auto info=Node_Conection::rest_client->get_api_core_v2_info();
    QObject::connect(info,&Node_info::finished,reciever,[=]( ){

        auto node_outputs_=new Node_outputs();

        connect(node_outputs_,&Node_outputs::finished,reciever,[=]( ){

            for(const auto& out: node_outputs_->outs_)
            {
                emit removed_expired(out.metadata().outputid_.toHexString());

            }
            info->deleteLater();
            node_outputs_->deleteLater();
        });
        Node_Conection::rest_client->
                get_outputs<Output::Basic_typ>(node_outputs_,
                                               "expirationReturnAddress="+Account::addr_bech32({0,0,0},info->bech32Hrp)+
                                               "&sender="+Account::addr_bech32({0,0,0},info->bech32Hrp));

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
