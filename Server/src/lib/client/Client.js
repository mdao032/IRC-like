"use strict";

import shortid from 'shortid';
import Socket from './../socket/socket';
import ERRSender from './../responses/ERRSender';
import RPLSender from './../responses/RPLSender';
import crypto from 'crypto';
import Redis from '../data/RedisInterface';

let clients = [];

class Client {
    /**
     * constructor
     * @param {Socket} socket
     */
    constructor(socket) {
        this._id = shortid.generate();
        this._name = null;
        this._identity = null;
        this._realname = null;
        this._socket = socket;
        this._socket.client = this;
        this._away = false;
        this._ip = socket.socket.remoteAddress;
        /**
         i - marque un utilisateur comme invisible ;
         s - marque un utilisateur comme recevant les notifications du serveur ;
         w - l'utilisateur reçoit les WALLOPs ;
         o - drapeau d'opérateur.
         s - drapeau super admin
         */
        this._flags = '';
        this._addFlag('sw');
        this._channels = [];
        this._pass = '';
        this._registeredWithPass = false;
        clients.push(this);
    }

    /**
     * change identity of user if its valid
     * @param {string} identity
     * @param {string} realname
     */
    setIdentity(identity, realname) {
        let error = false;
        let regexIdentity = /^([a-zA-Z0-9_-é"'ëäïöüâêîôûç`è]{1,15})$/.exec(identity);

        if (!regexIdentity || regexIdentity[1].indexOf('GUEST_') >= 0) {
            ERRSender.ERR_NEEDMOREPARAMS(this, 'USER');
            error = true;
        }
        if (this._identity) {
            ERRSender.ERR_ALREADYREGISTRED(this);
            error = true;
        }
        for (let i = 0; i < clients.length; i++) {
            if ((this._pass && clients[i].identity === identity) || (!this._pass && clients[i].identity === 'GUEST_' + identity)) {
                ERRSender.ERR_ALREADYREGISTRED(this);
                error = true;
            }
        }
        if (error) {
            return;
        }

        if (this._pass) {
            Redis.getUsers((users) => {
                if (!users) {
                    this._addFlag('O');
                } else {
                    if (users[identity]) {
                        let userFromMongo = JSON.parse(users[identity]);
                        if (userFromMongo.pass != this._pass) {
                            ERRSender.ERR_PASSWDMISMATCH(this);
                            return
                        }
                        this._flags = userFromMongo.flags;
                    }
                }
                this._socket.logger._CLIENT_LOGGED();
                this._identity = identity;
                this._registeredWithPass = true;
                this._realname = realname;
                if(!this._name) {
                    this.name = this._identity;
                }
                this._mergeToRedis();
                RPLSender.RPL_MOTDSTART(this.socket);
                RPLSender.RPL_MOTD(this.socket);
                RPLSender.RPL_ENDOFMOTD(this.socket);
            })
        } else {
            this._registeredWithPass = false;
            this._identity = 'GUEST_' + identity;
            this._socket.logger._CLIENT_GUEST();
            this._realname = realname;
            this._mergeToRedis();
            RPLSender.RPL_MOTDSTART(this.socket);
            RPLSender.RPL_MOTD(this.socket);
            RPLSender.RPL_ENDOFMOTD(this.socket);
        }
    }

    /**
     * set the user name
     * @param {string} name
     */
    set name(name) {
        let error = false;
        if (name === null) {
            this.socket.logger._USER_CHANGE_NICK('Guest_' + this._id);
            RPLSender.NICK(this.name, 'Guest_' + this._id, this);
            this._name = null;
            error = true;
        } else {
            if (name[0] === ':') {
                name = name.slice(1, name.length);
            }
            clients.forEach((c) => {
                if (c.name === name) {
                    if (!c.isRegisteredWithPass() && this.isRegisteredWithPass()) {
                        c.name = null;
                    } else {
                        ERRSender.ERR_NICKNAMEINUSE(this);
                        error = true;
                    }
                }
            });
            let match = name.match(/[a-zA-Z0-9_-é"'ëäïöüâêîôûç`è]+/);
            if (!match || (match && match[0] !== name) || name === '' || name.length > 15) {
                ERRSender.ERR_NONICKNAMEGIVEN(this);
                error = true;
            }
            if (!error) {
                this.socket.logger._USER_CHANGE_NICK(name);
                RPLSender.NICK(this.name, name, this);
                this._name = name;
            }
        }
    }

    /**
     * get user socket or null if its not defined
     * @returns {null|Socket}
     */
    get socket() {
        return this._socket;
    }

    /**
     *
     * @param {string} pass
     */
    set pass(pass) {
        if(this.isRegisteredWithPass()) {
            this._pass = crypto.createHash('sha256').update(pass).digest('base64');
            this._mergeToRedis();
            RPLSender.RPL_PASSCHANGED(this.socket);
        } else {
            this._pass = crypto.createHash('sha256').update(pass).digest('base64');
        }

    }

    /**
     *
     * @returns {string} pass
     */
    get pass() {
        return this._pass;
    }

    /**
     *
     * @param {boolean} away
     */
    set away(away) {
        this._away = away;
    }

    /**
     *
     * @returns {boolean} away
     */
    get away() {
        return this._away;
    }

    /**
     *
     * @returns {string} flags
     */
    get flags() {
        return this._flags;
    }

    /**
     * get user id
     * @returns {string}
     */
    get id() {
        return this._id;
    }

    /**
     *
     * @returns {Array<Channel>}
     */
    get channels() {
        return this._channels;
    }

    /**
     * get user ip, may be 127.0.0.1 if not defined
     * @returns {string}
     */
    get ip() {
        return this._ip || '127.0.0.1';
    }

    /**
     * get user identity, may be Guest_<id> if not defined
     * @returns {string}
     */
    get identity() {
        return this._identity || 'Guest_' + this._id;
    }

    /**
     * get user name, may be Guest_<id> if not defined
     * @returns {string}
     */
    get name() {
        return this._name || 'Guest_' + this._id;
    }

    /**
     * get user realname, may be Guest_<id> if not defined
     * @returns {string}
     */
    get realname() {
        return this._realname || 'Guest_' + this._id;
    }

    /**
     *
     * @param {string} val
     */
    set realname(val) {
        if (val[0] === ':') {
            val = val.slice(1, val.length);
        }
        this._realname = val;
    }

    /**
     * return true if user is registered / else false
     * @returns {boolean}
     */
    get isRegistered() {
        return this._identity !== null;
    }

    remove() {
        for (let i = 0; i < this._channels.length; i++) {
            this._channels[i].removeUser(this, 'Quit', true);
        }
        RPLSender.QUIT(this, 'Gone');
        if(clients.indexOf(this) >= 0)
            clients.splice(clients.indexOf(this), 1);
    };


    /**
     * remove channel from list
     * @param {Channel} channel
     */
    removeChannel(channel) {
        if(this._channels.indexOf(channel) >= 0)
            this._channels.splice(this._channels.indexOf(channel), 1);
    }

    /**
     * add channel from list
     * @param {Channel} channel
     */
    addChannel(channel) {
        this._channels.push(channel);
    }

    /**
     *
     * @param {string} flags
     * @private
     */
    _addFlag(flags) {
        let arrayFlags = flags.split('');
        arrayFlags.forEach((flag) => {
            if (this._flags.indexOf(flag) === -1) {
                this._flags += flag;
                this._mergeToRedis();
                if (flag === 'i' || flag === 'o') {
                    if (flag === 'o') {
                        this._channels.forEach((channel) => {
                            channel.changeClientFlag('+', 'o', this);
                        });
                        RPLSender.RPL_YOUREOPER(this.socket);
                    }
                    RPLSender.RPL_UMODEIS_BROADCAST_ALL(this.name + ' +' + flag);
                } else {
                    RPLSender.RPL_UMODEIS(this, this.name + ' +' + flag);
                }

            }
        });
    }

    /**
     *
     * @param {string} flags
     * @private
     */
    _removeFlag(flags) {
        let arrayFlags = flags.split('');
        arrayFlags.forEach((flag) => {
            let tmp = this._flags.length;
            this._flags = this._flags.replace(flag, '');
            if (tmp - 1 === this._flags.length) {
                this._mergeToRedis();
                if (flag === 'i' || flag === 'o') {
                    RPLSender.RPL_UMODEIS_BROADCAST_ALL(this.name + ' -' + flag);
                } else {
                    RPLSender.RPL_UMODEIS(this, this.name + ' -' + flag);
                }
            }
        });
    }

    /**
     *
     * @param {string} operator
     * @param {string} flag
     */
    changeFlag(operator, flag) {
        if (operator === '+') {
            this._addFlag(flag);
        } else {
            this._removeFlag(flag);
        }
    }

    /**
     *
     * @returns {boolean}
     */
    isRegisteredWithPass() {
        return this._registeredWithPass;
    }

    /**
     *
     * @returns {boolean}
     */
    isAdmin() {
        return (this._flags.indexOf('o') >= 0 || this._flags.indexOf('O') >= 0);
    }

    /**
     *
     * @returns {boolean}
     */
    isInvisible() {
        return this._flags.indexOf('i') > -1;
    }

    /**
     *
     * @returns {boolean}
     */
    isSuperAdmin() {
        return this._flags.indexOf('O') >= 0;
    }

    /**
     * if the client is connected with pass, we save in redis its id, its pass and  its flags
     * @private
     */
    _mergeToRedis() {
        if (this._registeredWithPass && this._identity) {
            Redis.setUser(this);
        }
    }

    /**
     *
     * @param {number|string|Client} id -> identifiant|nickname|client
     * @returns {null|Client}
     */
    static getClient(id) {
        for (let key in clients) {
            if (clients[key].name === id || key === id || clients[key].identity === id) {
                return clients[key];
            }
        }
        return null;
    }

    toString() {
        return this.name;
    }

    /**
     * get client list
     * @returns {Array<Client>}
     */
    static list() {
        return clients;
    }

}
export default Client;