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
var urlDetection = '';
if (isLocalHost) {
  urlDetection = '//' + host + ':3000/stash/detection?timestamp=' + Date.now();
} else {
  urlDetection = '//' + host + '/stash/detection?timestamp=' + Date.now();
}

// setup plotly
var layout = {
  autosize: false,
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
  width: document.getElementById('data').offsetWidth,
  height: document.getElementById('data').offsetHeight,
  plot_bgcolor: "rgba(0,0,0,0)",
  paper_bgcolor: "rgba(0,0,0,0)",
  annotations: [],
  displayModeBar: false,
  xaxis: {
    title: {
      text: xTitle,
      font: {
        size: 24
      }
    },
    showgrid: false,
    ticks: '',
    side: 'bottom'
  },
  yaxis: {
    title: {
      text: yTitle,
      font: {
        size: 24
      }
    },
    showgrid: false,
    ticks: '',
    ticksuffix: ' ',
    autosize: false,
    categoryorder: "total descending"
  }
};
var config = {
  displayModeBar: false,
  scrollZoom: true
}

// setup plotly data
var data = [
  {
    z: [[0, 0, 0], [0, 0, 0], [0, 0, 0]],
    colorscale: 'Jet',
    type: 'heatmap'
  }
];

Plotly.newPlot('data', data, layout, config);

// callback function
var intervalId = window.setInterval(function () {

  // check if timestamp is updated
  var timestampData = $.get(urlTimestamp, function () { })

    .done(function (data) {
      if (timestamp != data) {
        timestamp = data;

        // get new data
        var apiData = $.getJSON(urlDetection, function () { })
          .done(function (data) {

            // case draw new plot
            if (data.nRows != nRows) {
              nRows = data.nRows;

              var trace1 = {
                  x: data[xVariable],
                  y: data[yVariable],
                  mode: 'markers',
                  type: 'scatter'
              };
              
              var data_trace = [trace1];
              Plotly.newPlot('data', data_trace, layout, config);
            }
            // case update plot
            else {
              var trace_update = {
                x: [data[xVariable]],
                y: [data[yVariable]]
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
