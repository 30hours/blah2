const express = require('express');
const dgram = require('dgram');
const net = require("net");

// constants
const PORT = 3000;
const HOST = '0.0.0.0';
var map = '';
var detection = '';
var data = '';
var capture = false;

// api server
const app = express();
// header on all requests
app.use(function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "*");
  res.header('Cache-Control', 'private, no-cache, no-store, must-revalidate');
  res.header('Expires', '-1');
  res.header('Pragma', 'no-cache');
  next();
});
app.get('/', (req, res) => {
  res.send('Hello World');
});
app.get('/map', (req, res) => {
  res.send(map);
});
app.get('/detection', (req, res) => {
  res.send(detection);
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

// tcp listener map
const server_map = net.createServer((socket)=>{
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
server_map.listen(3001);

// tcp listener detection
const server_detection = net.createServer((socket)=>{
  socket.write("Hello From Server!")
  socket.on("data",(msg)=>{
      data = data + msg.toString();
      if (data.slice(-1) === "}")
      {
        console.log('EOF');
        detection = data;
        data = '';
      }
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_detection.listen(3002);
