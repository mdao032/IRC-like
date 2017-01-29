process.env.debug = true;

var def             = require('./modules/def');
var prompter        = require('./prompter.js');
var net             = require('net');
var config          = require('./config.json');
var socketManager   = require('./modules/socket/socket');
var Client          = require('./modules/client/client');
var Logger          = require('./modules/Logger');
var colors          = require('colors');

// true : coupe le serveur sur une erreur
// false : laisse le serveur tourner sur une erreur



socketManager.create(function(socket) {



    var c = new Client.client(socket);
    var logger = new Logger(c);

    socket.on('connect', function() {
        logger._CLIENT_CONNECTED();
        c.socket.send('Your name is '+c.id);
    });

    socket.on('message', function(str) {
        logger._CLIENT_SEND_MESSAGE(str);
        socket.broadcast(c.id + ' : '+ str);
    });

    socket.on('end', function() {
        logger._CLIENT_DECONNECTED();
        c.delete();
    });
});

var App = (function() {

    function App() {

    }

    App.prototype.query = function(str) {
        var req = str.split(' ');
        if(req[0] === 'clients') {
            if(Client.list().length > 0) {
                var ret = '';
                ret += '-- Client list --\n';
                for(var i=0; i<Client.list().length; i++) {
                    ret += i + '\t\t'+Client.list()[i].id+'\t\t'+Client.list()[i].socket.type+'\n';
                }

                console.log(colors.yellow(ret));
            } else {
                console.log(colors.yellow('no client connected'));
            }
        } else if (req[0] === 'send' && req[1] && req[2]) {
            var cli = Client.find(req[1]);
            if(cli) {
                var i = 2;
                ret = '';
                while(req[i]) {
                    ret += req[i]+' ';
                    i++;
                }
                cli.socket.send(ret+'\n');
                console.log(colors.green('YOU')+colors.white(' >> ')+ colors.yellow(cli.id)+ ' : '+colors.white(ret));
            } else {
                console.log(colors.yellow('Can\'t find this client...'));
            }
        } else if (req[0] === 'bc' && req[1]) {
            var i = 2;
            var ret = '';
            while(req[i]) {
                ret += req[i]+' ';
                i++;
            }
            Client.list().forEach(function(c) {
                c.socket.send(ret+'\n');
                console.log(colors.green('YOU')+colors.white(' >> ')+ colors.yellow(cli.id)+ ' : '+colors.white(ret));
            });
        }
    };

    return App;
})();



var app = new App();

prompter(app);