#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

// left
#include <QListWidget>
// right up
#include <QTextBrowser>
// right middle
#include <QTextEdit>
// right bottom
#include <QPushButton>

#include <QSplitter>
#include <QVBoxLayout>

#include "Chat.h"


/*
    负责界面更新，界面：
    1. 用户列表 userList
       显示所有用户名和ip地址信息
    2. 消息输出 msgShow
       显示消息
    3. 消息输入 msgInput
    4. 发送按钮 sendMsg
    5. 设置按钮 setup

    外部接口:
    void onNewUser(QString name, QString ip);
    当有新用户加入聊天，该函数用于更新界面

    void onNewContent(QString name, QString content, bool boardcast);
    当接收到新消息时，负责更新界面

    功能：
    用户输入消息，发送消息
*/
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void initUserList();

    Chat chat;

    // left
    QListWidget* userList;
    // right
    QTextBrowser* msgShow;
    QTextEdit* msgInput;
    QPushButton* sendMsg;
    QPushButton* setup;

signals:

public slots:
    void onSend();
    void onSetup();

    void onNewUser(QString name, QString ip);
    void onNewContent(QString name, QString content, bool boardcast);
};

#endif // MAINWINDOW_H
