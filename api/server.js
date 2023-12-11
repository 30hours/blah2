const express = require('express');
const dgram = require('dgram');
const net = require("net");

var data_map = require('./maxhold.js');
var data_detection = require('./detection.js');

// constants
const PORT = 3000;
const HOST = '0.0.0.0';
var map = '';
var detection = '';
var timestamp = '';
var timing = '';
var iqdata = '';
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
app.get('/api/map', (req, res) => {
  res.send(map);
});
app.get('/api/detection', (req, res) => {
  res.send(detection);
});
app.get('/api/timestamp', (req, res) => {
  res.send(timestamp);
});
app.get('/api/timing', (req, res) => {
  res.send(timing);
});
app.get('/api/iqdata', (req, res) => {
  res.send(iqdata);
});
app.get('/stash/map', (req, res) => {
  res.send(data_map.get_data_map());
});
app.get('/stash/detection', (req, res) => {
  res.send(data_detection.get_data_detection());
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
        detection = data;
        data = '';
      }
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_detection.listen(3002);

// tcp listener timestamp
const server_timestamp = net.createServer((socket)=>{
  socket.write("Hello From Server!")
  socket.on("data",(msg)=>{
    data = data + msg.toString();
    timestamp = data;
    data = '';
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_timestamp.listen(4000);

// tcp listener timing
const server_timing = net.createServer((socket)=>{
  socket.write("Hello From Server!")
  socket.on("data",(msg)=>{
    data = data + msg.toString();
    if (data.slice(-1) === "}")
    {
      timing = data;
      data = '';
    }
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_timing.listen(4001);

// tcp listener iqdata metadata
const server_iqdata = net.createServer((socket)=>{
  socket.write("Hello From Server!")
  socket.on("data",(msg)=>{
    data = data + msg.toString();
    if (data.slice(-1) === "}")
    {
      iqdata = data;
      data = '';
    }
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_iqdata.listen(4002);
