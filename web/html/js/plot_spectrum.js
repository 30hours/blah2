var timestamp = -1;
var nRows = 3;
var host = window.location.hostname;
var isLocalHost = is_localhost(host);
var range_x = [];
var range_y = [];

// setup API
var urlTimestamp;
var urlMap;
if (isLocalHost) {
  urlTimestamp = '//' + host + ':3000/api/timestamp';
} else {
  urlTimestamp = '/api/timestamp';
}
if (isLocalHost) {
  urlMap = '//' + host + ':3000' + '/stash/iqdata';
} else {
  urlMap = '/stash/iqdata';
}

// setup plotly
var layout = {
  autosize: true,
  margin: {
    l: 50,
    r: 50,
    b: 50,
    t: 10,
    pad: 0
  },
  hoverlabel: {
    namelength: 0
  },
  plot_bgcolor: "rgba(0,0,0,0)",
  paper_bgcolor: "rgba(0,0,0,0)",
  annotations: [],
  displayModeBar: false,
  xaxis: {
    title: {
      text: 'Frequency (MHz)',
      font: {
        size: 24
      }
    },
    ticks: '',
    side: 'bottom'
  },
  yaxis: {
    title: {
      text: 'Timestamp',
      font: {
        size: 24
      }
    },
    ticks: '',
    ticksuffix: ' ',
    autosize: false,
    categoryorder: "total descending"
  }
};
var config = {
  responsive: true,
  displayModeBar: false
}

// setup plotly data
var data = [
  {
    z: [[0, 0, 0], [0, 0, 0], [0, 0, 0]],
    colorscale: 'Jet',
    type: 'heatmap'
  }
];
var detection = [];

Plotly.newPlot('data', data, layout, config);

// callback function
var intervalId = window.setInterval(function () {

  // check if timestamp is updated
  $.get(urlTimestamp, function () { })

    .done(function (data) {
      if (timestamp != data) {
        timestamp = data;

        // get new map data
        $.getJSON(urlMap, function () { })
          .done(function (data) {

            // case draw new plot
            if (data.nRows != nRows) {
              nRows = data.nRows;
              // timestamp posix to js
              for (i = 0; i < data.timestamp.length; i++)
              {
                data.timestamp[i] = new Date(data.timestamp[i]);
              }
              var trace1 = {
                  y: data.timestamp,
                  z: data.spectrum,
                  colorscale: 'Jet',
                  zauto: false,
                  type: 'heatmap'
              };
              
              var data_trace = [trace1];
              Plotly.newPlot('data', data_trace, layout, config);
            }
            // case update plot
            else {
              // timestamp posix to js
              for (i = 0; i < data.timestamp.length; i++)
              {
                data.timestamp[i] = new Date(data.timestamp[i]);
              }
              var trace_update = {
                y: [data.timestamp],
                z: [data.spectrum]
              };
              Plotly.update('data', trace_update);
            }

          })
          .fail(function () {
          })
          .always(function () {
          });
      }
    })
    .fail(function () {
    })
    .always(function () {
    });
}, 100);
