#include "belote.h"
#include "ui_belote.h"

#include <QLabel>
#include <QMessageBox>
#include "rpl_response.h"

Belote::Belote(QWidget *parent, QTcpSocket *sock, QString chan, QString nick) :
    QMainWindow(parent),
    ui(new Ui::Belote),
    socket(sock),
    channelName(chan),
    username(nick)
{
    ui->setupUi(this);
    fold = new QDialog(this);
    QHBoxLayout *test = new QHBoxLayout();
    fold->setLayout(test);
    this->show();
    this->setWindowTitle(nick + " - " + "Belote room : " + chan);
    this->setObjectName("Belote");
    this->setStyleSheet("QMainWindow#Belote {" \
                        "border-image: url(\"ressources/img/tapis.jpg\") 0 0 0 0 stretch stretch;"
                        "}"
                        );
    lobbyWait();
}

Belote::~Belote()
{
    delete ui;
}

int Belote::position()
{
    return aPosition;
}

void Belote::position(int val)
{
    aPosition = val;
}

/*
 * User interface functions
 */

void Belote::clean()
{
    clearLayout(ui->south);
    clearLayout(ui->north);
    clearLayout(ui->east);
    clearLayout(ui->west);
}

void Belote::clearLayout(QLayout *layout)
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

void Belote::parse(QString string)
{
    qDebug() << "belote :" << string;
    if (!in_isTeamSelec(string))
    if (!in_isGameStart(string))
    if (!in_isFullTeam(string))
    if (!in_isGameReset(string))
    if (!in_isTrumpChoice(string))
    if (!in_isPlayerTake(string))
    if (!in_isTakeTurn(string))
    if (!in_isPlayerPlay(string))
    if (!in_isEndFold(string))
    if (!in_isCardDeal(string))
    if (!in_isRoundEnd(string))
    if (!in_isYourTurn(string))
        qDebug() << "cant find this";
}

void Belote::playCard()
{
    for(auto i:hand.keys()) {
        if (hand[i]->isDown()) {
            Card * toPlay = i;
            QString message = "BELOTE PLAY " + channelName + " " + QString::number(toPlay->code()) + '\n';
            socket->write(message.toUtf8());
            hand[i]->setIcon(QIcon());
            hand[i]->clearMask();
            ui->south->removeWidget(hand[i]);
            hand.remove(i);
        }
    }
}

void Belote::setInactive()
{
    for (auto i:hand.keys()) {
        hand[i]->setDisabled(true);
    }
}

void Belote::setActive(QString string)
{
    setInactive();
    QStringList cards = string.split(',');
    QList<int> iCards;
    for(auto i:cards) {
        iCards.push_back(i.toInt());
    }
    for (auto i:hand.keys()) {
        if (iCards.contains(i->code()))
            hand[i]->setEnabled(true);
    }
}

void Belote::displayCard(Card *card)
{
    qDebug() << card->rank() << card->suit() << card->value();
    QPixmap cards("ressources/img/cards.jpg");
    QRect rect(73*card->rank(), 97*card->suit(), 73, 97);
    QPixmap cropped = cards.copy(rect);
    cropped.scaledToHeight(70,Qt::SmoothTransformation);
    QLabel *lCard = new QLabel;
    lCard->setPixmap(cropped);
    ui->board->addWidget(lCard);
}

/*
 *
 */
void Belote::lobbyWait()
{
    QLabel *wait = new QLabel("Waiting for other players...");
    ui->buttons->addWidget(wait);
    wait->setStyleSheet("border-image : none;");
}

void Belote::receiveCard(int val)
{
    Card *card = new Card(val);
    QPixmap cards("ressources/img/cards.jpg");
    QRect rect(73*card->rank(), 97*card->suit(), 73, 97);
    QPixmap cropped = cards.copy(rect);
    cropped.scaledToHeight(70,Qt::SmoothTransformation);
    QPushButton *newCard = new QPushButton();
    newCard->setIcon(cropped);
    newCard->setStyleSheet("background-color: rgba( 255, 255, 255, 0%);");
    newCard->setIconSize(QSize(70, 90));
    ui->south->addWidget(newCard);
    hand[card] = newCard;
    connect(newCard , &QPushButton::pressed, this, &Belote::playCard);
}

void Belote::emptyHand()
{
    clean();
    for(auto i:hand.keys()) {
        hand.remove(i);
    }
}

void Belote::chooseTeam()
{
    clearLayout(ui->buttons);
    QMessageBox teamSelec;
    teamSelec.setText("Which team do you want to join ?");
    QPushButton *blueTeam = teamSelec.addButton(tr("Kotei"), QMessageBox::NoRole);
    QPushButton *redTeam = teamSelec.addButton(tr("Jbzz"), QMessageBox::NoRole);
    teamSelec.exec();
    if (teamSelec.clickedButton() == blueTeam) {
        QString message = "BELOTE READY " + channelName + " 0\n";
        socket->write(message.toUtf8());
    } else if (teamSelec.clickedButton() == redTeam){
        QString message = "BELOTE READY " + channelName + " 1\n";
        socket->write(message.toUtf8());
    }
    teamSelec.close();
}

bool Belote::in_isTrumpChoice(QString string)
{
    if (!string.contains(BELOTE::RPL::TRUMPIS))
        return false;
    int iCard = string.split(' ').last().toInt();
    Card *card = new Card(iCard);
    displayCard(card);
    return true;
}

void Belote::firstRound(int trump)
{
    CustomLayout *tmp = new CustomLayout();
    tmp->setLayout(ui->buttons);
    tmp->addButton("Take", trump / 8);
    tmp->addButton("No", -1);
    connect(tmp, &CustomLayout::isClicked, this, &Belote::take);
}

void Belote::take(int trump, CustomLayout *layout)
{
    QString message = "BELOTE TAKE " + channelName + " " + QString::number(trump) + '\n';
    qDebug() << "taking : " << message;
    socket->write(message.toUtf8());
    delete layout;
}

void Belote::secondRound(int trump)
{
    CustomLayout *tmp = new CustomLayout();
    tmp->setLayout(ui->buttons);
    if (trump != 0)
        tmp->addButton("Spades", 0);
    if (trump != 1)
        tmp->addButton("Hearths", 1);
    if (trump != 2)
        tmp->addButton("Clubs", 2);
    if (trump != 3)
        tmp->addButton("Diamonds", 3);
    if (aPosition != 3)
        tmp->addButton("No", -1);
    connect(tmp, &CustomLayout::isClicked, this, &Belote::take);
}

bool Belote::in_isCardDeal(QString string)
{
    if (!string.contains(BELOTE::RPL::RECEIVED))
        return false;
    QStringList cardList = string.split(' ').last().split(',');
    qDebug() << "cartes recues : " << cardList;
    for (auto i:cardList) {
        receiveCard(i.toInt());
    }
    setInactive();
    return true;
}

bool Belote::in_isPlayerTake(QString string)
{
    if (!string.contains(BELOTE::RPL::PLAYERTAKE))
        return false;
    clearLayout(ui->board);
    return true;
}

bool Belote::in_isPlayerPlay(QString string)
{
    if (!string.contains(BELOTE::RPL::PLAYED))
        return false;
    int card = string.split(' ').last().toInt();
    Card *played = new Card(card);
    displayCard(played);
    return true;
}

bool Belote::in_isEndFold(QString string)
{
    if (!string.contains(BELOTE::RPL::ENDFOLD))
        return false;
    QString cards = string.split(' ').last();
    QStringList cardList = cards.split(',');
    clearLayout(ui->board);
    lastFold.clear();
    fold->close();
    for (auto i:cardList)
        lastFold.append(new Card(i.toInt()));
    return true;
}

bool Belote::in_isTakeTurn(QString string)
{
    if (!string.contains(BELOTE::RPL::TURNTOTAKE))
        return false;
    QString turn = string.split(' ').last();
    QString strump = string.split(' ').at(3);
    int trump = strump.remove(0, 1).toInt();
    turn == "(0)" ? firstRound(trump) : secondRound(trump);
    return true;
}

bool Belote::in_isGameReset(QString string)
{
    if (!string.contains(BELOTE::RPL::GAMERESET))
        return false;
    emptyHand();
    lobbyWait();
    return true;
}

bool Belote::in_isTeamSelec(QString string)
{
    if (!string.contains(BELOTE::RPL::TEAMSELEC))
        return false;
    chooseTeam();
    return true;
}

bool Belote::in_isGameStart(QString string)
{
    if (!string.contains(BELOTE::RPL::ROUNDSTART))
        return false;
    QString players = string.split(' ').last();
    QStringList playerList = players.split(',');
    position(playerList.indexOf(username));
    QString eastPlayer = playerList.at((position() + 1) % 4);
    QString northPlayer = playerList.at((position() + 2) % 4);
    QString westPlayer = playerList.at((position() + 3) % 4);
    QLabel *east = new QLabel(eastPlayer);
    ui->east->addWidget(east);
    QLabel *north = new QLabel(northPlayer);
    ui->north->addWidget(north);
    QLabel *west = new QLabel(westPlayer);
    ui->west->addWidget(west);
    return true;
}

bool Belote::in_isYourTurn(QString string)
{
    if (!string.contains(BELOTE::RPL::YOURTURN))
        return false;
    clearLayout(ui->buttons);
    QString cardList = string.split(' ').last();
    qDebug() << "set active : " << cardList;
    setActive(cardList);
    return true;
}

bool Belote::in_isRoundEnd(QString string)
{
    if (!string.contains(BELOTE::RPL::ROUNDEND))
        return false;
    clearLayout(ui->south);
    return true;
}

bool Belote::in_isFullTeam(QString string)
{
    if (!string.contains(BELOTE::ERR::FULLTEAM))
        return false;
    QMessageBox::information(this, "Error", "Team is full");
    chooseTeam();
    return true;
}

void Belote::on_actionLast_Fold_triggered()
{
    QLayout *tmp = fold->layout();
    clearLayout(tmp);
    QPixmap cards("ressources/img/cards.jpg");
    for (auto i:lastFold) {
        QRect rect(73*i->rank(), 97*i->suit(), 73, 97);
        QPixmap cropped = cards.copy(rect);
        cropped.scaledToHeight(70,Qt::SmoothTransformation);
        QLabel *lCard = new QLabel;
        lCard->setPixmap(cropped);
        tmp->addWidget(lCard);
    }
    fold->show();
}
