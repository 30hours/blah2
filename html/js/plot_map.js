var timestamp = -1;
var nRows = 3;
var host = window.location.hostname;
var isLocalHost = (host === "localhost" || host === "127.0.0.1" || host === "192.168.0.112");
var range_x = [];
var range_y = [];

// setup API
var urlTimestamp;
var urlDetection;
if (isLocalHost) {
  urlTimestamp = '//' + host + ':3000/api/timestamp';
} else {
  urlTimestamp = '//' + host + '/api/timestamp';
}
if (isLocalHost) {
  urlDetection = '//' + host + ':3000/api/detection';
} else {
  urlDetection = '//' + host + '/api/detection';
}
if (isLocalHost) {
  urlMap = '//' + host + ':3000' + urlMap;
} else {
  urlMap = '//' + host + urlMap;
}
urlTimestamp = urlTimestamp + '?timestamp=' + Date.now();
urlDetection = urlDetection + '?timestamp=' + Date.now();
urlMap = urlMap + '?timestamp=' + Date.now();

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
      text: 'Bistatic Range (km)',
      font: {
        size: 24
      }
    },
    ticks: '',
    side: 'bottom'
  },
  yaxis: {
    title: {
      text: 'Bistatic Doppler (Hz)',
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

        // get detection data (no detection lag)
        var detectionData = $.getJSON(urlDetection, function () { })
          .done(function (data_detection) {
            detection = data_detection;
          });

        // get new map data
        var apiData = $.getJSON(urlMap, function () { })
          .done(function (data) {

            // case draw new plot
            if (data.nRows != nRows) {
              nRows = data.nRows;

              // lock range before other trace
              var layout_update = {
                'xaxis.range': [data.delay[0], data.delay.slice(-1)[0]],
                'yaxis.range': [data.doppler[0], data.doppler.slice(-1)[0]]
              };
              Plotly.relayout('data', layout_update);

              var trace1 = {
                  z: data.data,
                  x: data.delay,
                  y: data.doppler,
                  colorscale: 'Jet',
                  zauto: false,
                  zmin: 0,
                  zmax: Math.max(13, data.maxPower),
                  type: 'heatmap'
              };
              var trace2 = {
                  x: detection.delay,
                  y: detection.doppler,
                  mode: 'markers',
                  type: 'scatter',
                  marker: {
                    size: 16,
                    opacity: 0.6
                  }
              };
              
              var data_trace = [trace1, trace2];
              Plotly.newPlot('data', data_trace, layout, config);
            }
            // case update plot
            else {
              var trace_update = {
                x: [data.delay, detection.delay],
                y: [data.doppler, detection.doppler],
                z: [data.data, []],
                zmax: [Math.max(13, data.maxPower), []]
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
