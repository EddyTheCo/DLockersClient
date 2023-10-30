#pragma once

#include<QObject>
#include<QString>
#include <QtQml/qqmlregistration.h>
#include<booking.hpp>
#include<account.hpp>



class Book_Client : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString  server_id READ get_server_id WRITE set_server_id NOTIFY server_idChanged)
    Q_PROPERTY(ConState  state READ state WRITE set_state NOTIFY stateChanged)
    Q_PROPERTY(QString  clientId READ clientId NOTIFY clientIdChanged)
    Q_PROPERTY(QJsonObject  funds READ funds  NOTIFY fundsChanged)
    QML_ELEMENT
    QML_SINGLETON

public:
    Book_Client(QObject *parent = nullptr);
    enum ConState {
        Disconnected = 0,
        Sending,
        Connected
    };
    Q_ENUM(ConState)
    Q_INVOKABLE void restart(){
        monitor_state();
    }
    Q_INVOKABLE void send_booking(const QJsonArray books);
    Q_INVOKABLE void presentNft(const QString address);
    void set_server_id(QString server_id_m);
    QString get_server_id(void)const;
    void sendNft(Node_output out,c_array address);
    QString clientId(void)const{return clientId_;}
    void setClientId(QString clie){if(clie!=clientId_){clientId_=clie;emit clientIdChanged();}}
    QJsonObject funds(void)const{return funds_json;}

     static ConState state(void){return state_;}
     void set_state(ConState state_m){if(state_m!=state_){state_=state_m;emit stateChanged();}}
signals:
    void sent_book(QJsonArray books,QString outid);

    void got_new_booking(QJsonArray books);
    void server_idChanged();
    void stateChanged();
    void notEnought(QJsonObject);
    void removed_expired(QString);
    void clientIdChanged();
    void fundsChanged();

private:
    void checkFunds(std::vector<qiota::Node_output>  outs);
    void setFunds(quint64 funds_m);
    void monitor_state(void);
    void check_if_expired(void);
    void check_state_output(const Node_output &node_output_);
    void deserialize_state(const QByteArray &state);

    quint64 price_per_hour_;
    std::vector<Booking> books_;
    qblocks::c_array server_id_,paytoaddr_;
    QObject* reciever;
    static ConState state_;
    bool inited;
    QString clientId_;
    QJsonObject funds_json;
    quint64 funds_;
    QHash<QString,quint64> total_funds;
};
