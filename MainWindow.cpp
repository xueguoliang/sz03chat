#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    connect(&chat, SIGNAL(sigNewUser(QString,QString)),
                this, SLOT(onNewUser(QString,QString)));
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

            userList->removeItemWidget(item);
            userList->update();
            break;
        }
    }

    this->userList->addItem(name + "@" + ip);
}
