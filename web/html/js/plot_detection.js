var timestamp;
var nRows = 3;
var host = window.location.hostname;
var isLocalHost = is_localhost(host);
var range_x = [];
var range_y = [];

// setup API
var urlTimestamp;
var urlDetection;
if (isLocalHost) {
  urlTimestamp = '//' + host + ':3000/api/timestamp';
} else {
  urlTimestamp = '/api/timestamp';
}
if (isLocalHost) {
  urlDetection = '//' + host + ':3000/stash/detection';
} else {
  urlDetection = '/stash/detection';
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

              // timestamp posix to js
              if (xVariable === "timestamp")
              {
                for (i = 0; i < data[xVariable].length; i++)
                {
                  data[xVariable][i] = new Date(data[xVariable][i]);
                }
              }

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
              // timestamp posix to js
              if (xVariable === "timestamp")
              {
                for (i = 0; i < data[xVariable].length; i++)
                {
                  data[xVariable][i] = new Date(data[xVariable][i]);
                }
              }
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
