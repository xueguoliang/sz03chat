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



class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

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
    void onNewUser(QString name, QString ip);
};

#endif // MAINWINDOW_H
