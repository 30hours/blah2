const http = require('http');

var nCpi = 100;
var map = [];
var timestamp = [];
var delay = [];
var doppler = [];
var detection = '';
var ts = '';
var output = [];
const options_timestamp = {
  host: '127.0.0.1',
  path: '/api/timestamp',
  port: 3000
};
const options_detection = {
  host: '127.0.0.1',
  path: '/api/detection',
  port: 3000
};

function update_data() {

  // check if timestamp is updated
  http.get(options_timestamp, function(res) {
    res.setEncoding('utf8');
    res.on('data', function (body) {
      if (ts != body)
      {
        ts = body;
        http.get(options_detection, function(res) {
          let body_map = '';
          res.setEncoding('utf8');
          res.on('data', (chunk) => {
            body_map += chunk;
          });
          res.on('end', () => {
            try {
              detection = JSON.parse(body_map);
              map.push(detection);
              if (map.length > nCpi) {
                map.shift();
              }
              delay = [];
              doppler = [];
              timestamp = [];
              for (var i = 0; i < map.length; i++)
              {
                for (var j = 0; j < map[i].delay.length; j++)
                {
                  delay.push(map[i].delay[j]);
                  doppler.push(map[i].delay[j]);
                  timestamp.push(map[i].timestamp);
                }
              }
              output = {
                timestamp: timestamp,
                delay: delay,
                doppler: doppler
              };
            } catch (e) {
              console.error(e.message);
            }
          });
        });
      }
    });
  });

};

setInterval(update_data, 100);

function get_data() {
  return output;
};

module.exports.get_data_detection = get_data;