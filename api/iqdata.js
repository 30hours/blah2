const http = require('http');

var nCpi = 20;
var iqdata = '';
var spectrum = [];
frequency = [];
var timestamp = [];
var detection = '';
var ts = '';
var output = [];
const options_timestamp = {
  host: '127.0.0.1',
  path: '/api/timestamp',
  port: 3000
};
const options_iqdata = {
  host: '127.0.0.1',
  path: '/api/iqdata',
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
        http.get(options_iqdata, function(res) {
          let body_map = '';
          res.setEncoding('utf8');
          res.on('data', (chunk) => {
            body_map += chunk;
          });
          res.on('end', () => {
            try {
              output = JSON.parse(body_map);
              // spectrum
              spectrum.push(output.spectrum);
              if (spectrum.length > nCpi) {
                spectrum.shift();
              }
              output.spectrum = spectrum;
              // frequency
              frequency.push(output.frequency);
              if (frequency.length > nCpi) {
                frequency.shift();
              }
              output.frequency = frequency;
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

module.exports.get_data_iqdata = get_data;