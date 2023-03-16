#pragma once

#include<QObject>
#include<QString>
#include <QtQml/qqmlregistration.h>
#include<booking.hpp>
#include<account.hpp>
#include"nodeConnection.hpp"


class Book_Client : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString  server_id READ get_server_id WRITE set_server_id NOTIFY server_idChanged)
    Q_PROPERTY(QString  code_str READ get_code_str WRITE set_code_str NOTIFY code_str_changed)
    Q_PROPERTY(unsigned int  topay READ get_topay NOTIFY topay_changed)
    Q_PROPERTY(ConState  state READ state NOTIFY stateChanged)
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
    Q_INVOKABLE void try_to_book(const std::vector<Booking> &books);
    Q_INVOKABLE void restart(){
        monitor_state();
    }
    void set_server_id(QString server_id_m);
    QString get_server_id(void)const;
    QString get_code_str(void)const{return code_str_;}


    quint64 get_topay(void)const{return topay_;}

    void set_code_str(QString code_str_m){code_str_=code_str_m;emit code_str_changed();}


     static ConState state(void){return state_;}
     void set_state(ConState state_m){if(state_m!=state_){state_=state_m;emit stateChanged();}}
signals:
    void code_str_changed();
    void topay_changed();
    void sent_book(Booking);
    void removed_expired(Booking);
    void got_new_booking(Booking book);
    void server_idChanged();
    void stateChanged();


private:
    void send_booking(Booking book, std::vector<Node_output> &outputs, Node_info *info);

    void set_to_pay(quint64 price){if(price!=topay_){topay_=price;emit topay_changed();}}
    void monitor_state(void);
    void check_if_expired(void);
    void check_state_output(const Node_output &node_output_);




    QString code_str_;
    quint64 price_per_hour_,topay_;
    std::vector<Booking> books_;
    qblocks::c_array server_id_,paytoaddrhash_;
    QObject* reciever;
    static ConState state_;

};
