#include "channel.h"

#include <QListWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <QColor>
#include <QVBoxLayout>
#include <QLabel>
#include <QTime>
#include <QList>
/*
 * Channel: Constructor
 */

Channel::Channel(ParserEmoji *emoji):
    emoji(emoji)
{
    currentChannel = QString("\"Debug\"");
    channels[currentChannel] = ChannelContent();
    channels[currentChannel].addUser("The godly dev");
    channels[currentChannel].topic("Here we see debug command.");
}

/*
 * Getters
 */

 QList<Message> Channel::chatContent()
 {
     return channels[currentChannel].chatContent();
 }

/*
 * Channel: Channel creation functions
 */

void Channel::join(QString chan, QString topic)
{
    if (!channels.contains(chan)) {
        channels[chan] = ChannelContent();
        channels[chan].topic(topic);
    }
}

void Channel::joinWhisper(QString dest){
    if (!channels.contains(dest))
        channels[dest] = ChannelContent();
}

/*
 * Channel: Channel quit functions
 */

void Channel::leave(QString chan){
    if(channels.contains(chan) && chan.compare("\"Debug\"") != 0) {
        change("\"Debug\"");
        channels.remove(chan);
    }
}

/*
 * Channel: Text adding functions
 */

void Channel::appendCurrent(QString string, QString pseudo)
{
    QString time = '[' + QTime::currentTime().toString() + ']';
    if (channels.contains(currentChannel))
        channels[currentChannel].appendChat(time + "    ", pseudo , " : " + emoji->parse(string));
}

void Channel::appendChannel(QString string, QString channel, QString send)
{
    QString time = '[' + QTime::currentTime().toString() + ']';
    if (channels.contains(channel))
        channels[channel].appendChat(time + "    ", send," : " + emoji->parse(string));
}

void Channel::clean(){
    channels[currentChannel].clearContent();
}

/*
 * Channel: Current channel change function
 */

void Channel::change(QString newChannel)
{
    if (channels.contains(newChannel))
        currentChannel = newChannel;
}

/*
 * Channel: Current channel name getter
 */

QString Channel::channelName()
{
    return currentChannel;
}

QList<QString> Channel::channelNames()
{
    return channels.keys();
}

/*
 * User list functions
 */

void Channel::addUser(QString user, QString channel)
{
    if (channels.contains(channel))
        channels[channel].addUser(user);
}

void Channel::delUser(QString user, QString channel)
{
    if (channels.contains(channel))
        channels[channel].removeUser(user);
}

QList<QString> Channel::users()
{
    return channels[currentChannel].users();
}

void Channel::changeNick(QString nick, QString newNick)
{
    for (auto i:channels.keys()) {
        channels[i].replaceUser(nick, newNick);
    }
    if (channels.keys().contains(nick)) {
        channels[newNick] = channels[nick];
        channels.remove(nick);
    }
}

void Channel::topic(QString topic, QString channel)
{
    if (channels.contains(channel))
        channels[channel].topic(topic);
}

QString Channel::topic()
{
    return channels[currentChannel].topic();
}

bool Channel::notif(QString chan)
{
    if (channels.contains(chan))
        return channels[chan].notif();
    else return false;
}

void Channel::togleNotif(QString chan, bool newValue)
{
    if (channels.contains(chan))
            channels[chan].togleNotif(newValue);
}

Message Channel::getLast()
{
    return channels[currentChannel].getLast();
}
