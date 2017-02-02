
var shortid     = require('shortid');
var err         = require('./../SignalManager');

var channels = [];

var Channel = (function() {

    function Channel(creator, name, pass, maxSize) {
        if(!typeof maxSize === "number") {
            err.ERR_NOSUCHCHANNEL(creator);
            delete this;
            return;
        }
        this.flags = '';
        this.id = shortid.generate();
        this.creator = creator;
        this.pass = pass;
        this.maxSize = maxSize;

        this.invisibleUsers = [];
        this.notifiedUsers = [];
        this.operators = [];

        this.bannedUsers = [];
        this.users = [];

        this.invitation = [];

        this.operators.push(creator);
        this.users.push(creator);
        this.notifiedUsers.push(creator);
        creator.channels.push(this);


        this.name = '';
        this.setName(creator, name);

        if(this.name === '') {
            delete this;
            return;
        }

        channels.push(this);

        this.broadcast(':'+creator.name+' JOIN '+this.name);

    }

    Channel.prototype.setName = function(op, name) {
        if(this.operators.indexOf(op)<0) {
            err.ERR_NOTOPONCHANNEL(op.socket);
            return;
        }

        if(name[0] !== '#') {
            name = '#'+name;
        }
        var error = false;
        channels.forEach(function(chan) {
            if(chan.name === name) {
                error = true;
            }
        });
        if(error) {
            err.ERR_NOSUCHCHANNEL(op.socket);
            return;
        }
        this.name = name;
    }

    Channel.prototype.addUser = function(user, key) {
        if(this.bannedUsers.indexOf(user)>=0) {
            err.ERR_BANNEDFROMCHAN(user.socket);
            return;
        }
        if(this._isInvitation && this.invitation.indexOf(user) === -1) {
            err.ERR_INVITEONLYCHAN(user.socket);
            return;
        }
        if(key !== this.pass) {
            err.ERR_BADCHANNELKEY(user.socket);
            return;
        }
        if(this.users.length >= this.maxSize) {
            err.ERR_CHANNELISFULL(user.socket);
            return;
        }
        user.channels.push(this);
        this.users.push(user);
        this.notifiedUsers.push(user);

        this.broadcast(':'+user.name+' JOIN '+this.name);
    }

    Channel.prototype.removeUser = function(user) {
        console.log('remove user');
        if(this.users.indexOf(user)<0) {
            err.ERR_NOTONCHANNEL(user.socket);
            return;
        }

        this.users.splice(this.users.indexOf(user), 1);
        this.invisibleUsers.splice(this.invisibleUsers.indexOf(user), 1);
        this.notifiedUsers.splice(this.notifiedUsers.indexOf(user), 1);

        this.operators.splice(this.notifiedUsers.indexOf(user), 1);
        if(user === this.creator) {
            console.log('remove user creator');
            this.creator = this.operators[0];
        }
        this.broadcast(':'+user.name+' PART '+this.name);
        user.channels.splice(user.channels.indexOf(this),1);

        if(this.users.length <= 0) {
            console.log('delete channel');
            channels.splice(channels.indexOf(this), 1);
            delete this;
        }
    }

    Channel.prototype.broadcast = function(message, except) {
        this.users.forEach(function(u) {
            if(u !== except)
                u.socket.send(message);
        });
    }




    Channel.prototype.__defineGetter__('_isPrivate', function() {
        return (this.flags.indexOf('p')>=0);
    });
    Channel.prototype.__defineGetter__('_isSecret', function() {
        return (this.flags.indexOf('s')>=0);
    });
    Channel.prototype.__defineGetter__('_isInvitation', function() {
        return (this.flags.indexOf('i')>=0);
    });

    Channel.list = function() {
        return channels;
    }

    return Channel;
})();

module.exports = Channel;