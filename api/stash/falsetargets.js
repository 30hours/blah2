const http = require('http');

var falsetargets = [];
frequency = [];
var ts = '';
var output = [];
const options_falsetargets = {
  host: '127.0.0.1',
  path: '/api/falsetargets',
  port: 3000
};

function update_data() {

  // check if timestamp is updated
  http.get(options_falsetargets, function (res) {
    res.setEncoding('utf8');
    res.on('data', function (body) {
      if (ts != body) {
        ts = body;
        http.get(options_falsetargets, function (res) {
          let body_map = '';
          res.setEncoding('utf8');
          res.on('data', (chunk) => {
            body_map += chunk;
          });
          res.on('end', () => {
            try {

              output = JSON.parse(body_map);
              // false targets
              falsetargets.push(output.falsetargets);
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

module.exports.get_data_falsetargets = get_data;