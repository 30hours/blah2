var timestamp = -1;
var nRows = 3;
var host = window.location.hostname;
var isLocalHost = is_localhost(host);
var range_x = [];
var range_y = [];

// setup API
var urlTimestamp;
var urlDetection;
var adsbData = {};
var urlAdsbLink;
var urlConfig;
if (isLocalHost) {
  urlTimestamp = '//' + host + ':3000/api/timestamp';
} else {
  urlTimestamp = '/api/timestamp';
}
if (isLocalHost) {
  urlDetection = '//' + host + ':3000/api/detection';
} else {
  urlDetection = '/api/detection';
}
if (isLocalHost) {
  urlMap = '//' + host + ':3000' + urlMap;
} else {
  // urlMap is already relative (e.g. '/api/map')
}
if (isLocalHost) {
  urlAdsbLink = '//' + host + ':3000/api/adsb2dd';
} else {
  urlAdsbLink = '/api/adsb2dd';
}
if (isLocalHost) {
  urlConfig = '//' + host + ':3000/api/config';
} else {
  urlConfig = '/api/config';
}

// get truth flag
var isTruth = false;
$.getJSON(urlConfig, function () { })
.done(function (data_config) {
  if (data_config.truth.adsb.enabled === true) {
    isTruth = true;
    $.getJSON(urlAdsbLink, function () { })
    .done(function (data) {
      adsbData = data;
    })
  }
});

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
  },
  showlegend: false
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
var adsb = {};

Plotly.newPlot('data', data, layout, config);

// callback function
var intervalId = window.setInterval(function () {

  // check if timestamp is updated
  $.get(urlTimestamp, function () { })

    .done(function (data) {
      if (timestamp != data) {
        timestamp = data;

        // get detection data (no detection lag)
        $.getJSON(urlDetection, function () { })
          .done(function (data_detection) {
            detection = data_detection;
          });

        // get ADS-B data if enabled in config

// Replace the interval ADS-B section (lines ~130-141) with:
        // get ADS-B data if enabled in config
        if (isTruth) {
          $.getJSON(urlAdsbLink, function () { })
            .done(function (data_adsb) {
              adsb['delay'] = [];
              adsb['doppler'] = [];
              adsb['flight'] = [];
              for (const aircraft in data_adsb) {
                if ('doppler' in data_adsb[aircraft]) {
                  adsb['delay'].push(data_adsb[aircraft]['delay'])
                  adsb['doppler'].push(data_adsb[aircraft]['doppler'])
                  adsb['flight'].push(data_adsb[aircraft]['flight'])
                }
              }
            });
        }
        // get new map data
        $.getJSON(urlMap, function () { })
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
                  colorscale: 'Viridis',
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
              var trace3 = {
                x: adsb.delay,
                y: adsb.doppler,
                mode: 'markers',
                type: 'scatter',
                marker: {
                  size: 16,
                  opacity: 0.6
                }
            };
              
              var data_trace = [trace1, trace2, trace3];
              Plotly.newPlot('data', data_trace, layout, config);
            }
            // case update plot
            else {
              var trace_update = {
                x: [data.delay, detection.delay, adsb.delay],
                y: [data.doppler, detection.doppler, adsb.doppler],
                z: [data.data, [], []],
                zmax: [Math.max(13, data.maxPower), [], []],
                text: [[], [], adsb.flight]
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
