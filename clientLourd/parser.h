#ifndef PARSEUR_H
#define PARSEUR_H

#include "rpl_response.h"
#include "err_response.h"

#include <QString>

class QTcpSocket;
class Channel;
class Channellist;

class Parser {
public:

    // Initialisation functions
    void initialize(Channel *channel, QTcpSocket *socket, QString *nickname, Channellist *list);
    void setNickname(QString *nick);
    void sendToServer(QTcpSocket *socket, QString string);
    // Parsing functions
    bool out(QString string);
    void in(QString string);

private:
    // Out functions
    bool out_isNickMsg(QString string);
    bool out_isUserMsg(QString string);
    bool out_isJoinMsg(QString string);
    bool out_isNamesMsg(QString string);
    bool out_isPassMsg(QString string);
    bool out_isPartMsg(QString string);
    bool out_isQuitMsg(QString string);
    bool out_isListMsg(QString string);
    bool out_isCleanMsg(QString string);
    bool out_isDebugMsg(QString string);
    bool out_isModeMsg(QString string);
    bool out_isTopicMsg(QString string);
    bool out_isKickMsg(QString string);
    bool out_isWhoMsg(QString string);
    bool out_isWhoisMsg(QString string);
    bool out_isMsgMsg(QString string);
    bool out_isPrivMsg(QString string);

    // In functions
    bool in_isChanList(QString string);
    bool in_isNameList(QString string);
    bool in_isJoinNote(QString string);
    bool in_isPartNote(QString string);
    bool in_isPrivMesg(QString string);
    bool in_isWhisMesg(QString string);
    bool in_isNickEdit(QString string);
    bool in_isKickMesg(QString string);
    bool in_isPing(QString string);
    bool in_isListMesg(QString string);

private:
    // Pointer to the channel and socket created in mainframe
    Channel *channel;
    QTcpSocket *socket;
    QString *nickname;
    Channellist *listOfChannels;
};

#endif // PARSEUR_H