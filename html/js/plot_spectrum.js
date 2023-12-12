var timestamp = -1;
var nRows = 3;
var host = window.location.hostname;
var isLocalHost = (host === "localhost" || host === "127.0.0.1" || host === "192.168.0.112");
var range_x = [];
var range_y = [];

// setup API
var urlTimestamp = '';
if (isLocalHost) {
  urlTimestamp = '//' + host + ':3000/api/timestamp?timestamp=' + Date.now();
} else {
  urlTimestamp = '//' + host + '/api/timestamp?timestamp=' + Date.now();
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
  //scrollZoom: true
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
  var timestampData = $.get(urlTimestamp, function () { })

    .done(function (data) {
      if (timestamp != data) {
        timestamp = data;

        // get new map data
        var apiData = $.getJSON(urlMap, function () { })
          .done(function (data) {

            // case draw new plot
            if (data.nRows != nRows) {
              nRows = data.nRows;

              var trace1 = {
                  z: data.spectrum,
                  colorscale: 'Jet',
                  zauto: false,
                  //zmin: 0,
                  //zmax: Math.max(13, data.maxPower),
                  type: 'heatmap'
              };
              
              var data_trace = [trace1];
              Plotly.newPlot('data', data_trace, layout, config);
            }
            // case update plot
            else {
              var trace_update = {
                z: [data.spectrum]
                //zmax: [Math.max(13, data.maxPower), []]
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
