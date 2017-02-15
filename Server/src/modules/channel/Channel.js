"use strict";

import Client from './../client/client';
import ERRSender from './../responses/ERRSender';
import RPLSender from './../responses/RPLSender';

let channels = [];


class Channel {

    /**
     *
     * @param {Client} creator
     * @param {string} name
     * @param {string} pass
     * @param {number} size
     * @constructor
     */
    constructor(creator, name, pass, size) {
         /**
         s 		canal secret; le canal est totalement invisible
         p 		canal privé; le nom du canal est invisible
         n 		les messages externes ne sont pas autorisés
         m 		canal modéré, seuls les utilisateurs en mode +v et les opérateurs peuvent envoyer un message
         i 		canal accessible uniquement sur invitation (commande /invite)
         t 		sujet du canal uniquement modifiable par les opérateurs du canal
         */
        this._flags = '';
        this.addChannelFlag(['t', 'n']);
        this.pass = pass;
        this.size = size;
        this.bannedUsers = [];
        this._users = [];
        /**
         o 	@ 	nom de l'utilisateur concerné 	Opérateur de canal : peut changer les modes du channel et expulser les autres utilisateurs
         v 	+ 	nom de l'utilisateur concerné 	verbose ou voiced : autorise l'utilisateur à parler sur un canal modéré (mode +m)
         */
        this._usersFlags = {};
        this.invitation = [];
        this._name = name;
        channels.push(this);
        this.addUser(creator, pass);
    }

    /**
     *
     * @returns {string}
     */
    get name() {
        return this._name;
    }

    /**
     *
     * @returns {*}
     */
    get usersFlags() {
        return this._usersFlags;
    }

    /**
     * return user list of this channel
     * @returns {Array}
     */
    get users() {
        return this._users;
    }

    /**
     * get status private for the channel, true = private / false = public
     * @returns {boolean}
     */
    get isPrivate() {
        return (this._flags.indexOf('p') >= 0);
    }

    /**
     * return status secret for the channel, true = isSecret / false = visible
     * @returns {boolean}
     */
    get isSecret() {
        return (this._flags.indexOf('s') >= 0);
    }

    /**
     * return status invitation for the channel, true = isInvitation only / false = accessible
     * @returns {boolean}
     */
    get isInvitation() {
        return (this._flags.indexOf('i') >= 0);
    }

    /**
     *
     * @param {Array} flag
     */
    addChannelFlag(flag) {
        if(flag instanceof Array) {
            flag.forEach((f) => {
                this._flags = this._flags.split(f).join('') + f;
            });
        } else {
            throw "flag must be an Array";
        }
    }
    removeChannelFlag(flag) {
        if(flag instanceof Array) {
            flag.forEach((f) => {
                this._flags = this._flags.split(f).join('');
            });
        } else {
            throw "flag must be an Array";
        }
    }

    /**
     * return true if the clinet is operator
     * @param {Client} client
     * @returns {boolean}
     */
    isUserOperator(client) {
        if (this._usersFlags[client.id] && this._usersFlags[client.id].indexOf('o') >= 0)
            return true;
        return false;
    }

    /**
     * return true if the client is voice
     * @param {Client} client
     * @returns {boolean}
     */
    isUserVoice(client) {
        if (this._usersFlags[client.id] && this._usersFlags[client.id].indexOf('v') >= 0)
            return true;
        return false;
    }

    /**
     *
     * @param {Client} client
     */
    setUserOperator(client) {
        if (this._usersFlags[client.id].indexOf('o')<0) {
            this._usersFlags[client.id] += 'o';
        }
    }
    /**
     *
     * @param {Client} client
     */
    setUserVoice(client) {
        if (this._usersFlags[client.id].indexOf('v')<0) {
            this._usersFlags[client.id] += 'v';
        }
    }
    /**
     *
     * @param {Client} client
     */
    removeUserFlag(client, flag) {
        if(this._usersFlags[client.id]) {
            this._usersFlags[client.id] = this._usersFlags[client.id].split(flag).join('');
        }
    }



    /**
     * add user to the channel
     * @param {Client} user
     * @param {string} key
     */
    addUser(user, key) {
        if (this.bannedUsers.indexOf(user) >= 0) {
            ERRSender.ERR_BANNEDFROMCHAN(user, this);
            return;
        }
        if (this._isInvitation && this.isInvitation.indexOf(user) === -1) {
            ERRSender.ERR_INVITEONLYCHAN(user, this);
            return;
        }
        if (key !== this.pass) {
            ERRSender.ERR_BADCHANNELKEY(user, this);
            return;
        }
        if (this.users.length >= this.size) {
            ERRSender.ERR_CHANNELISFULL(user, this);
            return;
        }
        if(this._users.indexOf(user) < 0) {
            this._users.push(user);
            this._usersFlags[user.id] = '';

            if (this._users.length === 1) {
                this.setUserOperator(user);
            }

            if (this.pass.length>0){
                this.addChannelFlag(['p']);
            }

            if(user.isAdmin()) {
                this.setUserOperator(user);
            }

            user.addChannel(this);
            RPLSender.JOIN(user, this);
            RPLSender.RPL_TOPIC(user, this);
            RPLSender.RPL_NAMREPLY(user, this);
        }
    }

    /**
     * remove user from this channel
     * @param {Client} user
     */
    removeUser(user) {
        let index = this._users.indexOf(user);
        if (index < 0) {
            ERRSender.ERR_NOTONCHANNEL(user, this);
            return;
        }
        RPLSender.PART(user, this);
        user.removeChannel(this);
        delete this._usersFlags[user.id];
        this._users.splice(index, 1);
    }

    /**
     * Broadcast message to this channel, if except is defined this client don't receive the message
     * @param {string} message
     * @param {Client} except
     */
    broadcast(message, except) {
        this.users.forEach((u) => {
            if (u !== except)
                u.socket.send(message);
        });
    }

    /**
     *
     * @returns {Array<Channel>}
     */
    static list() {
        return channels;
    }

}

export default Channel;