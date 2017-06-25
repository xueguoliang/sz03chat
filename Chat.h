#ifndef CHAT_H
#define CHAT_H

#include <QObject>
#include <QList>
#ifdef WIN32
#include <winsock2.h>
#define socklen_t int
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <pthread.h>
#include <errno.h>

#include <QDebug>
#include <stdlib.h>
#include <unistd.h>
#include <QThread>
#include <QFileInfo>
#include <QFile>
#include "chatdef.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QProcessEnvironment>

#include <QMap>

class Chat;

typedef struct
{
    QString name;
    QString ip;
} User;

static int SendFileInfoId = 0;
typedef struct SendFileInfo
{
    QString filepath_src;  // 源路径
    QString filepath_dst;  // 目标路径
    int id;
    int peerid; // 这个传输信息，在对方的id是多少
    QString ip;
    int total;
    int transSize; // 显示百分比
    bool started;

    Chat* chat;
    int server_fd; // 服务器的socket
    int port;


    ~SendFileInfo()
    {
        if(server_fd != -1)
            close(server_fd);
    }

    SendFileInfo(Chat* ch)
    {
        server_fd = -1;
        port = -1;
        chat = ch;
        peerid = -1;
        started = false;
        transSize = 0;
        id = SendFileInfoId++; // 构造函数不是线程安全的，所以只有主线程才可以创建这个对象
    }
} SendFileInfo;

/*
    负责聊天业务逻辑和数据结构
    1. name：本机机器名，可修改
    2. ips：本机器ip地址列表，用于预防自己给自己发消息
    3. others：在线的其他人
    4. udp_fd：用于通信的socket，使用的是UDP（广播，局域网丢包可能性小）
    5. tid是线程id
    6. recv_thread和run：是线程执行函数，线程负责接收报文，解析报文，处理报文，界面更新通过信号通知界面模块
*/

class Chat : public QObject
{
    Q_OBJECT
public:
    explicit Chat(QObject *parent = nullptr);

    QString account;
    QStringList ips;
    // ip is key
    QMap<QString, User*> others;

  //  QMap<int, SendFileInfo*> sends; // 发送的文件信息
  //  QMap<int, SendFileInfo*> recvs; // 接收的文件信息
    QMap<int, SendFileInfo*> transInfo;
    int udp_fd;
    pthread_t tid;

    static void* recv_file_thread(void* ptr);
    static void* send_file_thread(void* ptr);

    void recv_one_file(SendFileInfo* info);
    void send_one_file(SendFileInfo* info);

    static void* recv_thread(void* ptr);
    void run();
    void handleMsg(const QJsonObject& obj, QString ip);
    void addUser(QString ip, QString name);

    // 初始化
    void init();
    QString getSysName();
    QStringList getSysIps();

    // 发送数据
    // send函数，两个线程都会调用
    void create_socket(QString ip);
    QString broadcast_ip;
    void send(const QJsonObject& obj, QString ip);
    void sendMsg(QString content, QString ip);
    void sendOnline();
    void sendFile(QString path, QString ip);
    void ackFileTransRequest(QString filename, int filesize, int peerid, QString ip, bool ok, QString dstFilename="");


signals:
    // 信号是用于通知界面模块
    void sigNewUser(QString name, QString ip);
    void sigNewContent(QString name, QString content, bool boardcast);
    void sigTransFileRequest(QString filename, int filesize, int peerid, QString ip);
    void sigAckTransFile(int id, int peerid, int port);
    void sigProgress(SendFileInfo* info);
    void sigTransFinish(SendFileInfo* info);

public slots:
    void onAcktransFile(int id, int peerid, int port);
};

#endif // CHAT_H
