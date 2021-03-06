"use strict";

import Channel from './../channel/Channel';
import Client from './../client/Client';
import ERRSender from './../responses/ERRSender';
import RPLSender from './../responses/RPLSender';
import Socket from './../../lib/socket/socket';

module.exports = function (socket, command) {
    if (!socket.client.isRegistered) {
        ERRSender.ERR_NOTREGISTERED(socket.client, 'MODE');
        return;
    }
    let channelModeRegex = /^(#[a-zA-Z0-9_-é"'ëäïöüâêîôûç`è]{1,15}) ([+|-])([opsitnmlbvk]{1,11})(?: ([a-zA-Z0-9_-é"'ëäïöüâêîôûç`è]{1,15})(?: ([0-9]{1,15})$|$)|$)/.exec(command[1]);
    let userModeRegex = /^([a-zA-Z0-9_-é"'ëäï.öüâêîôûç`è]{1,15}) ([+|-])([iswbo]{1,5})(?: ([0-9]{1,15})$|$)/.exec(command[1]);

    if (channelModeRegex) {
        let nameChannel = channelModeRegex[1];
        let sign = channelModeRegex[2];
        let flags = channelModeRegex[3];
        //arg4 can be nickame or channel limit or channel key
        let arg4 = channelModeRegex[4];
        let banishmentTime = channelModeRegex[5];
        let channel = Channel.getChannelByName(nameChannel);
        let user;
        let operator;

        if (!channel) {
            ERRSender.ERR_NOSUCHCHANNEL(socket.client, nameChannel);
            return;
        }
        operator = channel.getUser(socket.client.identity);
        if (!operator) {
            ERRSender.ERR_NOTONCHANNEL(socket.client, channel);
            return;
        }
        if (!channel.isUserOperator(operator)) {
            ERRSender.ERR_CHANOPRIVSNEEDED(socket.client, nameChannel);
            return;
        }
        if (arg4 && flags.indexOf('b') > -1) {
            let userBanned = Client.getClient(arg4);
            if (!userBanned) {
                ERRSender.ERR_NOSUCHNICK(socket.client, arg4);
                return;
            }
            if (sign === '+' && banishmentTime) {
                if (!channel.getUser(arg4)) {
                    ERRSender.ERR_USERNOTINCHANNEL(socket.client, arg4, channel.name);
                }
                if (userBanned.isAdmin() || userBanned.isSuperAdmin() || channel.isUserOperator(userBanned)) {
                    ERRSender.ERR_USERSDONTMATCH(socket.client);
                    return;
                }
                channel.ban(userBanned, banishmentTime);
            } else if (sign === '-') {
                channel.unban(userBanned);
            }
        }

        if (arg4 && (flags.indexOf('o') > -1 || flags.indexOf('v') > -1)) {
            user = channel.getUser(arg4);
            if (!user) {
                ERRSender.ERR_USERNOTINCHANNEL(socket.client, arg4, channel.name);
                return;
            }
            if ((channel.isUserOperator(user) && !user.isAdmin() && !user.isSuperAdmin()) && !(operator.isAdmin() || operator.isSuperAdmin())) {
                ERRSender.ERR_USERSDONTMATCH(socket.client);
                return;
            }
            if (user.isAdmin() && !operator.isSuperAdmin()) {
                ERRSender.ERR_USERSDONTMATCH(socket.client);
                return;
            }
            //give/take channel operator privileges;
            if (flags.indexOf('o') > -1) {
                /*Possibility of change*/
                if (!user.isRegisteredWithPass()) {
                    ERRSender.ERR_USERSDONTMATCH(socket.client);
                    return;
                }
                channel.changeClientFlag(sign, 'o', user);
            }
            //give/take the ability to speak on a moderated channel;
            if (flags.indexOf('v') > -1) {
                channel.changeClientFlag(sign, 'v', user);
            }
        } else if (flags.indexOf('k') > -1) {
            //set a channel key (password).
            if (sign === '-') {
                channel.setPass('');
                RPLSender.RPL_CHANNELMODEIS(channel, nameChannel + ' -k');
            } else if (arg4 && sign === '+') {
                if (!(channel.pass === '')) {
                    ERRSender.ERR_KEYSET(socket.client, channel.name);
                    return;
                } else {
                    channel.setPass(arg4);
                    RPLSender.RPL_CHANNELMODEIS(channel, nameChannel + ' +k ' + arg4);
                }
            }
        }
        if (flags.indexOf('l') > -1 && sign === '+' && arg4 && (!isNaN(parseFloat(arg4)) && isFinite(arg4)) && arg4 < 500 && arg4 > 1) {
            //set the user limit to channel;
            channel.setSize(arg4);
            RPLSender.RPL_CHANNELMODEIS(channel, nameChannel + ' +l ' + arg4);
        }
        if (flags.indexOf('p') > -1) {
            //private channel flag;
            channel.changeChannelFlag(sign, 'p');
        }
        if (flags.indexOf('s') > -1) {
            //secret channel flag;
            channel.changeChannelFlag(sign, 's');
        }
        if (flags.indexOf('i') > -1) {
            //invite-only channel flag;
            channel.changeChannelFlag(sign, 'i');
        }
        if (flags.indexOf('t') > -1) {
            //topic settable by channel operator only flag;
            channel.changeChannelFlag(sign, 't');
        }
        if (flags.indexOf('n') > -1) {
            //no messages to channel from clients on the outside;
            channel.changeChannelFlag(sign, 'n');
        }
        if (flags.indexOf('m') > -1) {
            //moderate channel, only user with v flag can speak
            channel.changeChannelFlag(sign, 'm');
        }
    } else if (userModeRegex) {
        //name user can be ip
        let nameUser = userModeRegex[1];
        let sign = userModeRegex[2];
        let flags = userModeRegex[3];
        let banDuration = userModeRegex[4];
        let user = Client.getClient(nameUser);

        if (flags.indexOf('b') > -1 && sign === '-') {
            Socket.unBan(nameUser);
            return;
        }
        if (!user) {
            ERRSender.ERR_NOSUCHNICK(socket.client, nameUser);
            return;
        }
        if (user === socket.client) {
            if (flags.indexOf('i') > -1) {
                user.changeFlag(sign, 'i');
            }
            if (flags.indexOf('s') > -1) {
                user.changeFlag(sign, 's');
            }
            if (flags.indexOf('w') > -1) {
                user.changeFlag(sign, 'w');
            }
            if (flags.indexOf('o') > -1 && sign === '-' && !socket.client.isSuperAdmin()) {
                user.changeFlag(sign, 'o');
            }
        } else if (user != socket.client) {
            /*Possibility of change*/
            if (!user.isRegisteredWithPass()) {
                ERRSender.ERR_USERSDONTMATCH(socket.client);
                return;
            }
            if (socket.client.isSuperAdmin()) {
                if (flags.indexOf('o') > -1) {
                    user.changeFlag(sign, 'o');
                }
                if (flags.indexOf('b') > -1 ) {
                    if (sign === '+' && banDuration) {
                        Socket.ban(user, banDuration);
                    }
                }
            } else if (socket.client.isAdmin() && !user.isAdmin() && !user.isSuperAdmin()) {
                if (flags.indexOf('o') > -1) {
                    user.changeFlag(sign, 'o');
                }
                if (flags.indexOf('b') > -1) {
                    if (sign === '+' && banDuration) {
                        Socket.ban(user, banDuration);
                    }
                }
            }
            else {
                ERRSender.ERR_USERSDONTMATCH(socket.client);
            }
        }
    } else {
        ERRSender.ERR_UMODEUNKNOWNFLAG(socket.client);
    }
};
