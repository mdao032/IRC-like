#ifndef MAINFRAME_H
#define MAINFRAME_H

#include "channel.h"
#include "parser.h"
#include "msglist.h"

#include <QMainWindow>
#include <QString>
#include <QList>

class QPixmap;
class QTcpSocket;
class QScrollBar;
class QPoint;
template <typename,typename>
class QHash;
class QStringList;
class QFileDialog;
class QStandardPaths;
class Channellist;

namespace Ui {
class MainFrame;
}

class MainFrame : public QMainWindow
{
    Q_OBJECT

public:
    //Constructor and Destructor
    explicit MainFrame(QWidget *parent = 0,QTcpSocket *socket=NULL,QString nick=NULL);
    ~MainFrame();
    //Nickname setter (switch slot implementation ?)
    void setNickname(QString nick);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

public slots:
    //Socket slots
    void readyRead();
    void closeEvent (QCloseEvent *event);

    //UI slots
    void on_pushButton_emojis_clicked();
    void moveScrollBarToBottom(int min, int max);

signals:
	void showLogin();

private slots:
    void on_channelList_itemSelectionChanged();
    void on_messageSender_returnPressed();
    void on_pushButton_send_customContextMenuRequested(const QPoint &pos);
    void on_pushButton_upload_clicked();

    void on_actionConnect_triggered();

private:
    Ui::MainFrame *ui;

    //Tcp pointer from login
    QTcpSocket *socket;

    //Parser and channel for message handling
    QString nickname;
    Parser parser;
    Channel channel;
    MsgList msgList;
    Channellist *chanList;
    QHash<QString, QPixmap> *emoji;
};

#endif // MAINFRAME_H