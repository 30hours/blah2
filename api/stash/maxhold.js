const http = require('http');

var nCpi = 20;
var map = [];
var maxhold = '';
var timestamp = '';
const options_timestamp = {
  host: '127.0.0.1',
  path: '/api/timestamp',
  port: 3000
};
const options_map = {
  host: '127.0.0.1',
  path: '/api/map',
  port: 3000
};

function process(matrixArray) {

  const result = [];

  for (let i = 0; i < matrixArray[0].length; i++) {
    const row = [];
    for (let j = 0; j < matrixArray[0][0].length; j++) {
        let maxVal = matrixArray[0][i][j];
        for (let k = 1; k < matrixArray.length; k++) {
            maxVal = Math.max(maxVal, matrixArray[k][i][j]);
        }
        row.push(maxVal);
    }
    result.push(row);
}

  return result;
}

function update_data() {

  // check if timestamp is updated
  http.get(options_timestamp, function(res) {
    res.setEncoding('utf8');
    res.on('data', function (body) {
      if (timestamp != body)
      {
        timestamp = body;
        http.get(options_map, function(res) {
          let body_map = '';
          res.setEncoding('utf8');
          res.on('data', (chunk) => {
            body_map += chunk;
          });
          res.on('end', () => {
            try {
              maxhold = JSON.parse(body_map);
              map.push(maxhold.data);
              if (map.length > nCpi) {
                map.shift();
              }
              maxhold.data = process(map);
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
  return maxhold;
};

module.exports.get_data_map = get_data;