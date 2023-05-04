const express = require('express');
const dgram = require('dgram');

// constants
const PORT = 3000;
const HOST = '0.0.0.0';
var map = '';
var data = '';
var capture = false;

// api server
const app = express();
// header on all requests
app.use(function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "*");
  next();
});
app.get('/', (req, res) => {
  res.send('Hello World');
});
app.get('/map', (req, res) => {
  res.send(map);
});
// read state of capture
app.get('/capture', (req, res) => {
  res.send(capture);
});
// toggle state of capture
app.get('/capture/toggle', (req, res) => {
  capture = !capture;
  res.send('{}');
});
app.listen(PORT, HOST, () => {
  console.log(`Running on http://${HOST}:${PORT}`);
});

// tcp listener
const net = require("net");
const server = net.createServer((socket)=>{
    socket.write("Hello From Server!")
    socket.on("data",(msg)=>{
        data = data + msg.toString();
        if (data.slice(-1) === "}")
        {
          console.log('EOF');
          map = data;
          data = '';
        }
    });
    socket.on("close",()=>{
        console.log("Connection closed.");
    })
});
server.listen(3001);
