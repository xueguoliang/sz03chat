#include "Chat.h"

Chat::Chat(QObject *parent) : QObject(parent)
{
    self.ip = getSysIp();
    self.name = getSysName();
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
    setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &arg, sizeof(arg));

    // 发送上线的广播
    /*
        {
            cmd: online,
            name: account-name
        }
    */
    QJsonObject obj;
    obj.insert(CMD, ONLINE);
    obj.insert(NAME, self.name);

    send(obj, BROADCAST);

    // 创建接收线程
    pthread_create(&tid, NULL, recv_thread, this);
}

// 运行在独立的线程上下文，Qt有规定，线程不能更新界面
void Chat::handleMsg(const QJsonObject &obj, QString ip)
{
    QString cmd = obj.value(CMD).toString();
    if(cmd == ONLINE)
    {
        QString name = obj.value(NAME).toString();

        addUser(ip, name);

        // 回应这个用户
        QJsonObject resp;
        resp.insert(CMD, ONLINEACK);
        resp.insert(NAME, self.name);
        send(resp, ip);
    }
    if(cmd == ONLINEACK)
    {
        QString name = obj.value(NAME).toString();
        addUser(ip, name);
    }
}

QString Chat::getSysName()
{
    return "hello";
}

QString Chat::getSysIp()
{
    return "";
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
