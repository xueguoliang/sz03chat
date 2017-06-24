#ifndef CHAT_H
#define CHAT_H

#include <QObject>
#include <QList>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <QDebug>

#include "chatdef.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#include <QMap>

typedef struct
{
    QString name;
    QString ip;
} User;

class Chat : public QObject
{
    Q_OBJECT
public:
    explicit Chat(QObject *parent = nullptr);

    User self;
    // ip is key
    QMap<QString, User*> others;

    int udp_fd;
    pthread_t tid;

    static void* recv_thread(void* ptr);
    void run();

    void init();
    void handleMsg(const QJsonObject& obj, QString ip);

    QString getSysName();
    QString getSysIp();
    void send(const QJsonObject& obj, QString ip);
    void sendMsg(QString content, QString ip, bool boardcast);

    void addUser(QString ip, QString name);


signals:
    void sigNewUser(QString name, QString ip);
    void sigNewContent(QString name, QString content, bool boardcast);

public slots:
};

#endif // CHAT_H
