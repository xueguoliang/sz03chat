#include "MainWindow.h"
#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QVariant>
MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    connect(&chat, SIGNAL(sigNewUser(QString,QString)),
                this, SLOT(onNewUser(QString,QString)));
    connect(&chat, SIGNAL(sigNewContent(QString,QString,bool)),
            this, SLOT(onNewContent(QString,QString,bool)));
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
    hBox->addWidget(sendMsg = new QPushButton("发送"));

    vBox->setMargin(0);
    hBox->setMargin(0);

    connect(sendMsg, SIGNAL(clicked()), this, SLOT(onSend()));
    connect(setup, SIGNAL(clicked()), this, SLOT(onSetup()));
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
    /* 得到 ip地址 */
    QString text = userList->currentItem()->text();
    qDebug() << "item text is:" << text; // name@ip

    QStringList stringList = text.split('@');
    if(stringList.length() != 2)
    {
        qDebug() << stringList;
        return;
    }
    QString ip = stringList.at(1);

    // 内容
    QString content = msgInput->toPlainText();
    if(content.length() == 0)
        return;

    chat.sendMsg(content, ip, false);

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

    chat.create_socket(combo->currentText());

    // 重新发一次上线
    chat.sendOnline();
}

void MainWindow::onNewContent(QString name, QString content, bool boardcast)
{
    msgShow->append(name + "说: "+ content);
}
