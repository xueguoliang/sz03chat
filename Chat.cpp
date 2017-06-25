#include "Chat.h"


Chat::Chat(QObject *parent) : QObject(parent)
{
    ips = getSysIps();
    account = getSysName();
    broadcast_ip = "255.255.255.255";
    udp_fd = -1;

    // 信号和槽，改变了执行代码的线程
    connect(this, SIGNAL(sigAckTransFile(int,int, int)),
            this, SLOT(onAcktransFile(int,int, int)));

#ifdef WIN32
    // 如果是windows环境下，初始化socket运行环境
    WSADATA wsaData;
    WSAStartup( MAKEWORD( 2, 1 ), &wsaData );
#endif
}

void *Chat::recv_file_thread(void *ptr)
{
    SendFileInfo* info = (SendFileInfo*) ptr;
    info->chat->recv_one_file(info);
    return NULL;
}

void *Chat::send_file_thread(void *ptr)
{
    SendFileInfo* info = (SendFileInfo*) ptr;
    info->chat->send_one_file(info);
    return NULL;
}

// 接收文件的事情
void Chat::recv_one_file(SendFileInfo *info)
{
#if 0
    create socket;
    bind();
    listen();
    accept();
    read();
#endif
    int newfd = accept(info->server_fd, NULL, NULL);
    if(newfd < 0)
        return;

    qDebug() << "recv connect " << newfd;

   QFile file(info->filepath_dst);
   file.open(QFile::WriteOnly);
    while(info->transSize < info->total)
    {
        char buf[4096];
        int ret = read(newfd, buf, sizeof(buf));
        if(ret > 0)
        {
            file.write(buf, ret);
            info->transSize += ret;

            emit sigProgress(info);
        }
        else if(ret < 0 && errno == EINTR)
        {
            continue;
        }
        else
            break;
    }

    qDebug() << "recv complete";
    file.close();
    close(newfd);

    emit sigTransFinish(info);
}

void Chat::send_one_file(SendFileInfo *info)
{
#if 0
    connect(server); // 是不是要知道端口号？
    openfile;
    readfile;
    while(1)
    {
        write socket;
    }
#endif
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(info->port);
    addr.sin_addr.s_addr = inet_addr(info->ip.toUtf8().data());

    if(::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0)
    {
        qDebug() << "connect file trans ok";
    }
    else
    {
        qDebug() << "connect filetrans error";
    }

    QFile file(info->filepath_src);
    file.open(QFile::ReadOnly);

    while(info->transSize < info->total)
    {
        QByteArray buf = file.read(4096);
        if(buf.length() == 0)
            break;

        info->transSize += buf.size();

        emit sigProgress(info);
        write(fd, buf.data(), buf.size());
    }

    qDebug() << "trans complete";

    close(fd);
    file.close();
    emit sigTransFinish(info);
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
        else
        {
            QThread::msleep(100);
        }
    }
}

void Chat::init()
{
    create_socket("0.0.0.0");

    sendOnline();

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
        resp.insert(NAME, this->account);
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
#if 0
    QJsonObject obj;
    obj.insert(CMD, SENDFILE);
    obj.insert(NAME, info.fileName());
    obj.insert(FILESIZE, filesize);
    obj.insert(ID, sfInfo->id);
#endif
    if(cmd == SENDFILE)
    {
        QString filename = obj.value(NAME).toString();
        int filesize = obj.value(FILESIZE).toInt();
        int peerid = obj.value(ID).toInt();

       //  SendFileInfo* info = new SendFileInfo; // 在子进程中，不能创建SendFileInfo对象
        emit this->sigTransFileRequest(filename, filesize, peerid, ip);

        qDebug() << "recv sendfile request" << filename << filesize << peerid;

    }
    if(cmd == SENDFILEACK)
    {
        QString ok = obj.value(RESULT).toString();
#if 0
        QJsonObject obj;
        obj.insert(CMD, SENDFILEACK);
        obj.insert(RESULT, OK);
        obj.insert(ID, info->peerid);
        obj.insert(PEERID, info->id);
        send(obj, ip);
#endif
        if(ok == OK)
        {

            // 创建发送线程
            int id = obj.value(ID).toInt();
         //   sends[id];
            int peerid = obj.value(PEERID).toInt();
            int port = obj.value(PORT).toInt();

            qDebug() << "recv sendfileack" << id << peerid << port;

            emit sigAckTransFile(id, peerid, port);


        }
        else
        {
            // 将SendFileInfo结构给删除
        }
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

void Chat::create_socket(QString ip)
{
    if(this->udp_fd != -1)
        close(this->udp_fd);

    this->udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(this->udp_fd < 0)
    {
        qDebug() << "error create socket";
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9988);
    addr.sin_addr.s_addr = inet_addr(ip.toUtf8().data());

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

void Chat::sendMsg(QString content, QString ip)
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
  //  obj.insert(BROADCAST1, boardcast);
    obj.insert(BROADCAST1, ip.indexOf("255")!=-1);
    obj.insert(CONTENT, content);
    obj.insert(NAME, account);
    send(obj, ip);

}

void Chat::sendOnline()
{
    foreach(QString ip, others.keys())
    {
        delete others[ip];
    }
    others.clear();

    QJsonObject obj;
    obj.insert(CMD, ONLINE);
    obj.insert(NAME, account);

    send(obj, broadcast_ip);
}

void Chat::sendFile(QString path, QString ip)
{
    QFile file(path);
    file.open(QFile::ReadOnly);
    int filesize = file.size();
    file.close();

    QFileInfo info(path);

    // 本端保存以下这个信息
    SendFileInfo* sfInfo = new SendFileInfo(this);

    QJsonObject obj;
    obj.insert(CMD, SENDFILE);
    obj.insert(NAME, info.fileName());
    obj.insert(FILESIZE, filesize);
    obj.insert(ID, sfInfo->id);

    send(obj, ip);

    sfInfo->filepath_src = path;
    sfInfo->total = filesize;

    // 保存在列表中
    transInfo[sfInfo->id] = sfInfo;
}

// 在主线程中运行
void Chat::ackFileTransRequest(QString filename, int filesize,
                               int peerid, QString ip, bool ok, QString dstFilename)
{
    if(ok)
    {
        qDebug() << "sendfile request ack ok:" << ip <<filesize << peerid << dstFilename;

        SendFileInfo* info = new SendFileInfo(this);
        info->filepath_src = filename;
        info->filepath_dst = dstFilename;// TODO
        info->peerid = peerid;
        info->total = filesize;
        info->ip = ip;
        transInfo[info->id] = info;

        info->server_fd = socket(AF_INET, SOCK_STREAM, 0);
        // 不绑定，就是服务默认给了一个端口
        listen(info->server_fd, 10);

        // 获取端口号
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        getsockname(info->server_fd, (sockaddr*)&addr, &len);
        qDebug() << "port is:" << ntohs(addr.sin_port);

        pthread_t t;
        pthread_create(&t, NULL, recv_file_thread, info);
        pthread_detach(t);

        QJsonObject obj;
        obj.insert(CMD, SENDFILEACK);
        obj.insert(RESULT, OK);
        obj.insert(ID, info->peerid);
        obj.insert(PEERID, info->id);
        obj.insert(PORT, (int)ntohs(addr.sin_port));
        send(obj, ip);


    }
    else
    {
        QJsonObject obj;
        obj.insert(CMD, SENDFILEACK);
        obj.insert(RESULT, CANCEL);
        obj.insert(ID, peerid);
        send(obj, ip);
    }
}

/* 这个槽函数在主线程中执行 */
void Chat::onAcktransFile(int id, int peerid, int port)
{
    SendFileInfo* info = transInfo[id];
    info->port = port;
    info->peerid = peerid;

    pthread_t t;
    pthread_create(&t, NULL, send_file_thread, info);
    pthread_detach(t);
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
