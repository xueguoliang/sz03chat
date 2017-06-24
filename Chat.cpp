#include "Chat.h"
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QProcessEnvironment>
Chat::Chat(QObject *parent) : QObject(parent)
{
    ips = getSysIps();
    account = getSysName();

#ifdef WIN32
    // 如果是windows环境下，初始化socket运行环境
    WSADATA wsaData;
    WSAStartup( MAKEWORD( 2, 1 ), &wsaData );
#endif
}

void *Chat::recv_thread(void *ptr)
{
    Chat* This = (Chat*)ptr;

    This->run();
    return NULL;
}

void Chat::run()
{
    while(1)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        char buf[4096];

        int ret = recvfrom(udp_fd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &len);
        if(ret > 0)
        {
            buf[ret] = 0; // 对方没有发送\0，所以自己加
            QJsonDocument doc = QJsonDocument::fromJson(QByteArray(buf));

           // char ip[16];
           // inet_ntoa_r(addr.sin_addr, ip, sizeof(ip));

            handleMsg(doc.object(), inet_ntoa(addr.sin_addr));
        }
        else if(ret < 0 && errno != EINTR)
        {
            qDebug() << "errno recv";
            exit(2);
        }
    }
}

void Chat::init()
{
    this->udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(this->udp_fd < 0)
    {
        qDebug() << "error create socket";
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9988);
    addr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(udp_fd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret != 0)
    {
        qDebug() << "error bind";
        exit(1);
    }

    // 设置该socket，可以发送广播
    int arg = 1;
#ifdef WIN32
    setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, (char*)&arg, sizeof(arg));
#else
    setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &arg, sizeof(arg));
#endif
    // 发送上线的广播
    /*
        {
            cmd: online,
            name: account-name
        }
    */
    QJsonObject obj;
    obj.insert(CMD, ONLINE);
    obj.insert(NAME, account);

    send(obj, "192.168.19.255");

    // 创建接收线程
    pthread_create(&tid, NULL, recv_thread, this);
}

// 运行在独立的线程上下文，Qt有规定，线程不能更新界面
void Chat::handleMsg(const QJsonObject &obj, QString ip)
{
    if(ips.indexOf(ip)!=-1)
    {
        qDebug() << "自己发送数据给自己，丢弃";
        return;
    }

    QString cmd = obj.value(CMD).toString();
    if(cmd == ONLINE)
    {
        QString name = obj.value(NAME).toString();

        addUser(ip, name);

        // 回应这个用户
        QJsonObject resp;
        resp.insert(CMD, ONLINEACK);
        resp.insert(NAME, name);
        send(resp, ip);
    }
    if(cmd == ONLINEACK)
    {
        QString name = obj.value(NAME).toString();
        addUser(ip, name);
    }
    if(cmd == CHAT)
    {
        bool broadcast = obj.value(BROADCAST1).toBool();
        QString content = obj.value(CONTENT).toString();
        QString name = obj.value(NAME).toString();

        emit this->sigNewContent(name, content, broadcast);
    }
}

QString Chat::getSysName()
{
#ifdef WIN32
    return QProcessEnvironment::systemEnvironment().value("USERNAME");
#else
    char buf[1024];
    memset(buf,0, sizeof(buf));
    FILE* fp = popen("whoami", "r");
    int ret = fread(buf, 1, sizeof(buf), fp);
    buf[ret-1] = 0;
    fclose(fp);
    return QString(buf);
#endif
}

QStringList Chat::getSysIps()
{
    QStringList ret;
    QList<QNetworkAddressEntry> entrys;

    QList<QNetworkInterface> infs = QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface inf, infs)
    {
        entrys.append(inf.addressEntries());
    }
    foreach(QNetworkAddressEntry entry, entrys)
    {
        if(entry.ip().toString().length() != 0)
            ret.append(entry.ip().toString());

#if 0
        if(entry.broadcast().toString().isEmpty())
            continue;
        qWarning() << "ip and broadcast ip:"
                   << entry.ip().toString()
                   << entry.broadcast().toString();
        items[ITEM_SHARE_ADDR]->addItem(entry.broadcast().toString());
#endif
    }
    return ret;
}

void Chat::send(const QJsonObject &obj, QString ip)
{
    QByteArray buf = QJsonDocument(obj).toJson();

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9988);
    addr.sin_addr.s_addr = inet_addr(ip.toUtf8().data());//0xffffffff;
    sendto(udp_fd, buf.data(), buf.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
}

void Chat::sendMsg(QString content, QString ip, bool boardcast)
{
#if 0
    ```
    {
        cmd: "chat",
        broadcast: true,
        content: "msg-body"
    }
    ```
#endif
    QJsonObject obj;
    obj.insert(CMD, CHAT);
    obj.insert(BROADCAST1, boardcast);
    obj.insert(CONTENT, content);
    obj.insert(NAME, account);
    send(obj, ip);

}

void Chat::addUser(QString ip, QString name)
{
    User* user = new User;
    user->ip = ip;
    user->name = name;

    if(others[ip]) delete others[ip];
    // add user
    others[ip] = user;

    emit sigNewUser(name, ip);
}
