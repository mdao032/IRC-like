#include "mainframe.h"
#include "ui_mainframe.h"

#include <QCloseEvent>
#include <QCompleter>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTimer>

#include "channellist.h"
#include "helpdialog.h"
#include "uploadfile.h"
#include "uploadwindow.h"
#include "../channel/message.h"
#include "../config/theme.h"

/*
 * Mainframe: constructor and destructor
 */

MainFrame::MainFrame(QWidget *parent, QTcpSocket *socket, QString host, int port) :
    QMainWindow(parent),
    ui(new Ui::MainFrame),
    socket(socket),
    host(host),
    port(port),
    channel(&parserEmoji),
    stringCompleter(nullptr),
    emoteCompleter(nullptr),
    pongResponse(true)
{
    initUiConf();
    initConnect();
    initUIStyle();
    initCompletion();
    listOfChannels = new Channellist(NULL, socket);
    parser.initialize(&channel, socket, User("Guest"), listOfChannels);
    msgList.msgSender(ui->messageSender);
    channelModified();
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainFrame::ping);
    timer->start(15000);
}

MainFrame::~MainFrame()
{
    delete ui;
    delete stringCompleter;
}

/*
 * Initialisation functons
 */


void MainFrame::printMsgLine(Message chatMsgLine)
{
    QHBoxLayout *pseudoBox = new QHBoxLayout;
    QHBoxLayout *chatLine = new QHBoxLayout;
    pseudoBox->setSpacing(2);
    chatLine->setSpacing(2);

    QLabel *LHeure = new QLabel(chatMsgLine.date());
    LHeure->setStyleSheet("color : " + theme->hour() + ';');
    LHeure->setFixedHeight(20);
    pseudoBox->addWidget(LHeure);

    QLabel *lPseudo= new QLabel(chatMsgLine.sender());
    if (!chatMsgLine.sender().compare(parser.nickname())) {
        lPseudo->setStyleSheet("color : " + theme->self() + ';');
    } else {
        lPseudo->setStyleSheet("color : " + theme->nick() + ';');
    }
    lPseudo->setFixedHeight(20);
    pseudoBox->addWidget(lPseudo);
    ui->nickBox->addLayout(pseudoBox);
    QLabel *lMessage = new QLabel(chatMsgLine.message());
    lMessage->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    lMessage->setOpenExternalLinks(true);
    lMessage->setFixedHeight(20);
    chatLine->addWidget(lMessage);
    chatLine->addStretch(0);
    ui->chatBox->addLayout(chatLine);
}

void MainFrame::connectSocket()
{
    connect(socket, &QTcpSocket::readyRead, this, &MainFrame::readyRead);
}

void MainFrame::printMsg(QList<Message> chatMsgList)
{
    clean();
    for (auto i:chatMsgList) {
        printMsgLine(i);
    }
}


void MainFrame::clearLayout(QLayout *layout)
{
    QLayoutItem *item;
    while((item = layout->takeAt(0))) {
        if (item->layout()) {
            clearLayout(item->layout());
            delete item->layout();
        }
        if (item->widget()) {
            delete item->widget();
        }
    }
}

void MainFrame::clean()
{
    QLayoutItem *item;
    while ((item = ui->chatBox->takeAt(0))) {
        clearLayout(item->layout());
    }
    while ((item = ui->nickBox->takeAt(0))) {
        clearLayout(item->layout());
    }
}

/*
 *  Parser related slots
 */

void MainFrame::channelModified()
{

    ui->channelList->clear();
    for (auto i:channel.channelNames()) {
        if (!i.startsWith('&')) {
            ui->channelList->addItem(i);
            if (channel.notif(i))
                ui->channelList->findItems(i,Qt::MatchExactly)[0]->setForeground(QColor("red"));
        }
    }
}

void MainFrame::userModified()
{
    ui->userList->clear();
    for (auto i:channel.users()) {
        if (!i->modeI()){
            QString name = i->name();
            if (i->modeO() || channel.oper(i))
                name.prepend('@');
            if (i->modeM())
                name.prepend('-');
            ui->userList->addItem(name);
        }
    }
    refreshMentionList();
}

void MainFrame::refreshMentionList()
{
    delete emoteCompleter;
    //Emotes completion
    QStringList emoteList = parserEmoji.keys();
    for (auto i:channel.userList()) {
        emoteList << '@' + i;
    }
    emoteCompleter = new QCompleter(emoteList,this);
    emoteCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    stringCompleter->popup()->setTabKeyNavigation(true);
    ui->messageSender->setMultipleCompleter(emoteCompleter);
}

void MainFrame::chatModified()
{
    printMsg(channel.chatContent());
}

void MainFrame::needClean()
{
    clean();
    channel.clean();
}

void MainFrame::changeChannel()
{
    chatModified();
    userModified();
    topicModified();
    ui->messageSender->setPlaceholderText("Message " + channel.channelName());
    ui->actionBip_on_messages->setChecked(channel.soundNotif(channel.channelName()));
}

void MainFrame::lineAdded()
{
    printMsgLine(channel.getLast());
}

void MainFrame::topicModified()
{
    ui->topicDisplay->setText(channel.topic());
}

void MainFrame::nickModified(QString nick)
{
    this->setWindowTitle(nick + '@' + host + ":" + QString::number(port));
}

/*
 * MainFrame: socket slots
 */

void MainFrame::readyRead()
{
    while(socket->canReadLine()){
        QString string = QString(socket->readLine());
        parser.in(string);
    }
}

void MainFrame::closeEvent (QCloseEvent *event)
{
    emit showLogin();
    if(parser.out("/quit"))
    {
        clean();
        event->accept();
    }
	emit deleteMainFrame(this);
}

/*
 * mainFrame: UI slots
 */
void MainFrame::on_pushButton_emojis_clicked()
{
    QMenu contextMenu(tr("Context menu"), this);
    QList<QAction* > listAction;
    QStringList emojis(parserEmoji.keys());
    emojis.sort(Qt::CaseInsensitive);
    for (auto i : emojis)
    {
        listAction.append(new QAction(i, this));
        listAction.last()->setIcon(QPixmap(parserEmoji.value(i)));
    }
    contextMenu.addActions(listAction);
    contextMenu.setMinimumSize(50, 80);
    QAction * action = contextMenu.exec(QCursor::pos());
    QString emotes;
    if (action)
        emotes = action->text();
    ui->messageSender->setText(ui->messageSender->text() + emotes);
    ui->messageSender->setFocus();
}

void MainFrame::moveScrollBarToBottom(int min, int max)
{
    Q_UNUSED(min);
    ui->scrollArea->verticalScrollBar()->setValue(max);
}

void MainFrame::on_channelList_itemSelectionChanged()
{
    QList<QListWidgetItem *> selected = ui->channelList->selectedItems();
    if (!selected.isEmpty())
        channel.change(selected.first()->text());
    if (channel.notif(channel.channelName())) {
        channel.togleNotif(channel.channelName(), false);
        channelModified();
    }
    ui->messageSender->setFocus();
    changeChannel();
}


bool MainFrame::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->messageSender) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Up) {
                msgList.scrollUp();
                return true;
            } else if (keyEvent->key() == Qt::Key_Down) {
                msgList.scrollDown();
                return true;
            } else if (keyEvent->key() == Qt::Key_Escape) {
                msgList.scrollReset();
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(obj, event);
    }
    return false;
}

void MainFrame::on_messageSender_returnPressed()
{
    QString message = ui->messageSender->text();
    msgList.addMsg(message);
    if(!parser.out(message))
        this->close();
    msgList.scrollReset();
    ui->messageSender->setText("");
    ui->messageSender->setPlaceholderText("Message " + channel.channelName());
}

void MainFrame::on_pushButton_upload_clicked()
{
    QTcpSocket *tmp = new QTcpSocket();
    tmp->connectToHost(host, 8090);
    if (!tmp->waitForConnected(5000))
        QMessageBox::information(this, "Error", "Host not found");
    QStringList homePath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    QStringList files = QFileDialog::getOpenFileNames(this, "Select one or more files to open", homePath.first());
    if (files.size() != 0) {
        UploadWindow *progress = new UploadWindow(this, tmp);
        Upload *up = new Upload(this, host, files, tmp);
        connect(progress, &UploadWindow::resultReady, this, &MainFrame::handleResults);
        connect(up, &Upload::finished, up, &QObject::deleteLater);
        up->start();
        progress->show();
        ui->messageSender->setFocus();
    }
}

void MainFrame::handleResults(QString url)
{
    QString message = "PRIVMSG " + channel.channelName() + " :" + url;
    parser.sendToServer(socket, message);
    url.remove(url.length() - 1, 1);
    channel.appendCurrent(url, parser.userNick());
    chatModified();
}

void MainFrame::on_actionConnect_triggered()
{
    emit showLogin();
}

void MainFrame::on_actionDisconnect_triggered()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Disconnect", "Are you sure ?", QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        this->close();
        emit showLogin();
		emit deleteMainFrame(this);
    }
}

void MainFrame::on_actionDark_toggled(bool arg1)
{
    ui->actionLight->setChecked(!arg1);
    theme->change(1);
    initUIStyle();
    chatModified();
}

void MainFrame::on_actionLight_toggled(bool arg1)
{
    ui->actionDark->setChecked(!arg1);
    theme->change(0);
    initUIStyle();
    chatModified();
}

void MainFrame::initUiConf()
{
    ui->setupUi(this);
    ui->actionDark->setCheckable(true);
    ui->actionLight->setCheckable(true);
    ui->actionSet_Away->setCheckable(true);
    ui->actionBip_on_messages->setCheckable(true);
    ui->actionHide_join_part_messages->setCheckable(true);
    ui->messageSender->installEventFilter(this);
    ui->pushButton_emojis->setIcon(QPixmap("ressources/img/smile.png"));
    ui->pushButton_upload->setIcon(QPixmap("ressources/img/upload.png"));
    ui->pushButton_emojis->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->messageSender->setFocus();
}

void MainFrame::initUIStyle()
{
    theme = ThemeList::instance();
    this->setStyleSheet("background-color : " + theme->background() + ';' + " color : " + theme->text() + ';');
    ui->channelList->setStyleSheet(
        "QListWidget::item { height : 35px; } \
        QListWidget::item:selected {background : qlineargradient(x1:0, y1:1, x2:1, y2:1, \
            stop:0 " + theme->gradStart() + ", stop:0.6 " + theme->gradEnd() + "); } \
        QListWidget::item:selected { border-left: 2px solid rgb(114, 137, 218); }"
    );
    ui->userList->setStyleSheet(
        "QListWidget::item { height : 35px; } \
        QListWidget::item:hover { background-color : rgb(46, 49, 54); }\
        QListWidget::item:selected { background-color : rgb(46, 49, 54); }"
    );
}

void MainFrame::initConnect()
{
	// Ui related connects
    connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged, this, &MainFrame::moveScrollBarToBottom);

    // Parser related connects
    connect(&parser, &Parser::channelModifiedSignal, this, &MainFrame::channelModified);
    connect(&parser, &Parser::userModifiedSignal, this, &MainFrame::userModified);
    connect(&parser, &Parser::chatModifiedSignal, this, &MainFrame::chatModified);
    connect(&parser, &Parser::cleanSignal, this, &MainFrame::needClean);
    connect(&parser, &Parser::changeChannelSignal, this, &MainFrame::changeChannel);
    connect(&parser, &Parser::topicModifiedSignal, this, &MainFrame::topicModified);
    connect(&parser, &Parser::lineAddedSignal, this, &MainFrame::lineAdded);
    connect(&parser, &Parser::nickModifiedSignal, this, &MainFrame::nickModified);

    connect(ui->actionhelp, &QAction::triggered, this, &MainFrame::help);

    // PingPong related connects
    connect(&parser, &Parser::pongSignal, this, &MainFrame::onPongSignal);
}

void MainFrame::initCompletion()
{
    //Commands completion
    QStringList completionList;
    completionList << "/clean" << "/debug" << "/nick" << "/user" << "/join" << "/names"
                   << "/pass" << "/part" << "/list" << "/topic" << "/kick" << "/who"
                   << "/whois" << "/mode" << "/msg" << "/quit" << "/away" << "/back"
                   << "/invite" << "/files" << "/rmfile" << "/belote" << "/serverkick"
                   <<"/rmchan" << "/mute" << "/unmute";
    stringCompleter = new QCompleter(completionList,this);
    stringCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    stringCompleter->popup()->setTabKeyNavigation(true);
    ui->messageSender->setCompleter(stringCompleter);


}

void MainFrame::on_userList_doubleClicked(const QModelIndex &index)
{
    QString user = ui->userList->item(index.row())->text();
    if(user[0] == '@')
        user.remove(0, 1);
    channel.joinWhisper(user);
    channelModified();
    channel.change(user);
    changeChannel();
}

void MainFrame::on_actionClean_triggered()
{
    channel.clean();
}

void MainFrame::on_actionSet_Away_toggled(bool arg1)
{
    ui->actionSet_Away->setChecked(arg1);
    if (arg1){
        parser.sendToServer(socket, "AWAY :Gone\n");
        channel.change("\"Debug\"");
    }
    else
        parser.sendToServer(socket, "AWAY\n");
}

void MainFrame::on_actionNames_triggered()
{
    parser.sendToServer(socket, "NAMES " + channel.channelName() + '\n');
    channel.change("\"Debug\"");
}

void MainFrame::on_actionwho_triggered()
{
    parser.sendToServer(socket, "WHO " + channel.channelName() + '\n');
    channel.change("\"Debug\"");
}

void MainFrame::on_actionClose_triggered()
{
    if (channel.channelName().startsWith('#'))
        parser.sendToServer(socket, "PART " + channel.channelName() + '\n');
    else {
        for (int i = 0; i < ui->channelList->count(); i++) {
            if (ui->channelList->item(i)->text() == channel.channelName())
                ui->channelList->takeItem(i);
        }
    }
}

void MainFrame::on_actionBip_on_messages_toggled(bool arg1)
{
    ui->actionBip_on_messages->setChecked(arg1);
    channel.soundNotif(channel.channelName(), arg1);
}

void MainFrame::on_actionHide_join_part_messages_toggled(bool arg1)
{
    ui->actionHide_join_part_messages->setChecked(arg1);
    channel.hideNotif(channel.channelName(), arg1);
}

void MainFrame::on_actionchannelList_triggered()
{
    socket->write("LIST\n");
    listOfChannels->show();
    listOfChannels->initUIStyle();
    listOfChannels->clear();
}

void MainFrame::onPongSignal()
{
    pongResponse = true;
}

void MainFrame::ping()
{
    if (!pongResponse) {
        socket->connectToHost(host, 8088);
        if (!socket->waitForConnected(5000))
            close();
    }
    pongResponse = false;
    socket->write(QString("PING\n").toUtf8());
}

void MainFrame::help(bool arg1)
{
    (void)arg1;
    HelpDialog *helpDialog = new HelpDialog(this);
    helpDialog->show();
}
