#include "MainWindow.h"
#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QVariant>
#include <QFileDialog>
#include <QMessageBox>
MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    connect(&chat, SIGNAL(sigNewUser(QString,QString)),
                this, SLOT(onNewUser(QString,QString)));
    connect(&chat, SIGNAL(sigNewContent(QString,QString,bool)),
            this, SLOT(onNewContent(QString,QString,bool)));
    connect(&chat, SIGNAL(sigTransFileRequest(QString,int,int,QString)),
            this, SLOT(onTransFileRequest(QString,int,int,QString)));
    connect(&chat, SIGNAL(sigProgress(SendFileInfo*)),
            this, SLOT(onTransProgress(SendFileInfo*)));
    connect(&chat, SIGNAL(sigTransFinish(SendFileInfo*)),
            this, SLOT(onTransFinish(SendFileInfo*)));

    chat.init();

    QVBoxLayout* m = new QVBoxLayout(this);

    QSplitter* splitter = new QSplitter(this);
    splitter->addWidget(userList = new QListWidget);
    m->addWidget(splitter);

    QWidget* container;
    splitter->addWidget(container = new QWidget);

    QVBoxLayout* vBox = new QVBoxLayout(container);
    QHBoxLayout* hBox;
    vBox->addWidget(this->msgShow = new QTextBrowser, 3);
    vBox->addWidget(this->msgInput = new QTextEdit, 1);
    vBox->addLayout(hBox = new QHBoxLayout);

    hBox->addStretch(1);
    hBox->addWidget(setup = new QPushButton("设置"));
    hBox->addWidget(sendFile = new QPushButton("发送文件"));
    hBox->addWidget(sendMsg = new QPushButton("发送"));

    vBox->setMargin(0);
    hBox->setMargin(0);

    connect(sendMsg, SIGNAL(clicked()), this, SLOT(onSend()));
    connect(setup, SIGNAL(clicked()), this, SLOT(onSetup()));
    connect(sendFile, SIGNAL(clicked()), this, SLOT(onSendFile()));

    initUserList();
}

void MainWindow::initUserList()
{
    this->userList->clear();

    this->userList->addItem("所有人@"+chat.broadcast_ip);
}

QString MainWindow::getSelectIp()
{
    /* 得到 ip地址 */
    QString text = userList->currentItem()->text();
    qDebug() << "item text is:" << text; // name@ip

    QStringList stringList = text.split('@');
    if(stringList.length() != 2)
    {
        qDebug() << stringList;
        return "";
    }
    QString ip = stringList.at(1);
    return ip;
}

void MainWindow::onNewUser(QString name, QString ip)
{
    int count = this->userList->count();
    for(int i=0;i <count; ++i)
    {
        QListWidgetItem* item = userList->item(i);

        // cccddd.indexOf("ab");
        if(item->text().indexOf(ip) != -1)
        {
            qDebug() << "remove same user";

          // userList->removeItemWidget(item);
          //  userList->update();
            delete userList->takeItem(i);
            break;
        }
    }

    this->userList->addItem(name + "@" + ip);
}

void MainWindow::onSend()
{
    // 取得选择的对方ip地址
    QString ip = getSelectIp();

    // 内容
    QString content = msgInput->toPlainText();
    if(content.length() == 0)
        return;

    chat.sendMsg(content, ip);

    // 整理界面
    msgInput->clear();
    msgShow->append("我说："+content);
}

void MainWindow::onSetup()
{
    QDialog dlg;
    QComboBox* combo;
    QHBoxLayout* lay = new QHBoxLayout(&dlg);
    lay->addWidget(new QLabel("选择网卡"));
    lay->addWidget(combo = new QComboBox());


    {
        QList<QNetworkAddressEntry> entrys;

        QList<QNetworkInterface> infs = QNetworkInterface::allInterfaces();
        foreach(QNetworkInterface inf, infs)
        {
            entrys.append(inf.addressEntries());
        }
        foreach(QNetworkAddressEntry entry, entrys)
        {
            if(entry.broadcast().toString().isEmpty())
                continue;

          //  entry.broadcast(); //广播地址
          //  entry.ip(); // ip地址


            combo->addItem(entry.ip().toString());
            combo->setItemData(combo->count()-1, entry.broadcast().toString());
        }
    }



    dlg.exec();

    // 得到当前用户的选择，并且赋值给chat对象
    chat.broadcast_ip = combo->itemData(combo->currentIndex()).toString();

 //   chat.create_socket(combo->currentText());

    // 重新发一次上线
    chat.sendOnline();

    // userList
    initUserList();
}

void MainWindow::onSendFile()
{
    QString ip = getSelectIp();

    QString filename = QFileDialog::getOpenFileName(NULL, "请选择要发送的文件");
    if(filename.length() == 0)
        return;
    qDebug() << filename;

    chat.sendFile(filename, ip);
}

void MainWindow::onNewContent(QString name, QString content, bool boardcast)
{
    if(boardcast)
        msgShow->append(name + "对大家说："+ content);
    else
        msgShow->append(name + "对我说：" + content);
}

void MainWindow::onTransFileRequest(QString filename, int filesize, int peerid, QString ip)
{
    // 弹一个对话框问用户是否接收文件
#if 0
    QMessageBox::warning();
    QMessageBox::information();
    QMessageBox::critical();
    QMessageBox::question();
#endif

    QString msg = QString("%1 给你发文件(%2)，要不要接收?").arg(chat.others[ip]->name, filename);
    if(QMessageBox::question(this, "有人给发文件", msg) == QMessageBox::Yes)
    {
        QString dstFilename = QFileDialog::getSaveFileName(this);
        if(dstFilename.length() == 0)
        {
            chat.ackFileTransRequest(filename, filesize, peerid, ip, false);
            return;
        }

        chat.ackFileTransRequest(filename, filesize, peerid, ip, true, dstFilename);
    }
    else
    {
        chat.ackFileTransRequest(filename, filesize, peerid, ip, false);
    }
}

void MainWindow::onTransProgress(SendFileInfo *info)
{
    qDebug() <<"trans progress:" << info->transSize << info->total;
}

void MainWindow::onTransFinish(SendFileInfo *info)
{
    if(info->server_fd == -1)
        qDebug() << "send complete:" << info->filepath_src;
    else
        qDebug() << "recv complete:" << info->filepath_dst;

    chat.transInfo.remove(info->id);
    delete info;
}
