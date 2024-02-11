const express = require('express');
const net = require("net");
const fs = require('fs');
const yaml = require('js-yaml');
const dns = require('dns');

// parse config file
var config;
try {
  const file = process.argv[2];
  config = yaml.load(fs.readFileSync(file, 'utf8'));
} catch (e) {
  console.error('Error reading or parsing the YAML file:', e);
}

var stash_map = require('./stash/maxhold.js');
var stash_detection = require('./stash/detection.js');
var stash_iqdata = require('./stash/iqdata.js');
var stash_timing = require('./stash/timing.js');

// constants
const PORT = config.network.ports.api;
const HOST = config.network.ip;
var map = '';
var detection = '';
var track = '';
var timestamp = '';
var timing = '';
var iqdata = '';
var data_map;
var data_detection;
var data_tracker;
var data_timestamp;
var data_timing;
var data_iqdata;
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
app.get('/api/tracker', (req, res) => {
  res.send(track);
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
app.get('/api/config', (req, res) => {
  res.send(config);
});
app.get('/api/adsb2dd', (req, res) => {
  if (config.truth.adsb.enabled == true) {
    const api_url = "http://adsb2dd.30hours.dev/api/dd";
    const api_query =
      api_url +
      "?rx=" + config.location.rx.latitude + "," +
      config.location.rx.longitude + "," +
      config.location.rx.altitude +
      "&tx=" + config.location.tx.latitude + "," +
      config.location.tx.longitude + "," +
      config.location.tx.altitude +
      "&fc=" + (config.capture.fs / 1000000) +
      "&server=" + "http://" + config.truth.adsb.ip;
    res.send(api_query);
  }
  else {
    res.status(400).end();
  }
});

// stash API
app.get('/stash/map', (req, res) => {
  res.send(stash_map.get_data_map());
});
app.get('/stash/detection', (req, res) => {
  res.send(stash_detection.get_data_detection());
});
app.get('/stash/iqdata', (req, res) => {
  res.send(stash_iqdata.get_data_iqdata());
});
app.get('/stash/timing', (req, res) => {
  res.send(stash_timing.get_data_timing());
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
    socket.on("data",(msg)=>{
        data_map = data_map + msg.toString();
        if (data_map.slice(-1) === "}")
        {
          map = data_map;
          data_map = '';
        }
    });
    socket.on("close",()=>{
        console.log("Connection closed.");
    })
});
server_map.listen(config.network.ports.map);

// tcp listener detection
const server_detection = net.createServer((socket)=>{
  socket.on("data",(msg)=>{
      data_detection = data_detection + msg.toString();
      if (data_detection.slice(-1) === "}")
      {
        detection = data_detection;
        data_detection = '';
      }
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_detection.listen(config.network.ports.detection);

// tcp listener tracker
const server_tracker = net.createServer((socket)=>{
  socket.on("data",(msg)=>{
      data_tracker = data_tracker + msg.toString();
      if (data_tracker.slice(-1) === "}")
      {
        track = data_tracker;
        data_tracker = '';
      }
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_tracker.listen(config.network.ports.track);

// tcp listener timestamp
const server_timestamp = net.createServer((socket)=>{
  socket.on("data",(msg)=>{
    data_timestamp = data_timestamp + msg.toString();
    timestamp = data_timestamp;
    data_timestamp = '';
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_timestamp.listen(config.network.ports.timestamp);

// tcp listener timing
const server_timing = net.createServer((socket)=>{
  socket.on("data",(msg)=>{
    data_timing = data_timing + msg.toString();
    if (data_timing.slice(-1) === "}")
    {
      timing = data_timing;
      data_timing = '';
    }
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_timing.listen(config.network.ports.timing);

// tcp listener iqdata metadata
const server_iqdata = net.createServer((socket)=>{
  socket.on("data",(msg)=>{
    data_iqdata = data_iqdata + msg.toString();
    if (data_iqdata.slice(-1) === "}")
    {
      iqdata = data_iqdata;
      data_iqdata = '';
    }
  });
  socket.on("close",()=>{
      console.log("Connection closed.");
  })
});
server_iqdata.listen(config.network.ports.iqdata);

