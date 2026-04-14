var timestamp = -1;
var nRows = 3;
var host = window.location.hostname;
var isLocalHost = is_localhost(host);
var range_x = [];
var range_y = [];

// setup API
var urlTimestamp;
var urlDetection;
var urlAdsb;
var urlAdsbLink;
var urlConfig;
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
if (isLocalHost) {
  urlAdsbLink = '//' + host + ':3000/api/adsb2dd';
} else {
  urlAdsbLink = '//' + host + '/api/adsb2dd';
}
if (isLocalHost) {
  urlConfig = '//' + host + ':3000/api/config';
} else {
  urlConfig = '//' + host + '/api/config';
}

// get truth flag
var isTruth = false;
$.getJSON(urlConfig, function () { })
.done(function (data_config) {
  if (data_config.truth.adsb.enabled === true) {
    isTruth = true;
    $.getJSON(urlAdsbLink, function () { })
    .done(function (data) {
      urlAdsb = data.url;
      if (!is_localhost(new URL(urlAdsb).hostname)) {
        urlAdsb = urlAdsb.replace(/^http:/, 'https:');
      }
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
  },
  {
    x: [],
    y: [],
    text: [],
    mode: 'markers',
    type: 'scatter',
    marker: {
      size: 16,
      opacity: 0.8,
      color: 'rgba(255, 165, 0, 0.8)'
    },
    hovertemplate: 'Range: %{x}<br>Doppler: %{y}',
    name: 'Blah2 Target'
  },
  {
    x: [],
    y: [],
    text: [],
    mode: 'markers',
    type: 'scatter',
    marker: {
      size: 14,
      opacity: 0.6,
      color: 'rgba(0, 200, 0, 0.8)'
    },
    hovertemplate: 'ADS-B Target: %{text}<br>Range: %{x}<br>Doppler: %{y}',
    name: 'ADS-B'
  },
  {
    x: [],
    y: [],
    text: [],
    mode: 'markers',
    type: 'scatter',
    marker: {
      size: 18,
      opacity: 1.0,
      color: 'rgba(255, 255, 0, 1)'
    },
    hovertemplate: 'ADS-B Target: %{text}<br>Range: %{x}<br>Doppler: %{y}',
    name: 'Selected ADS-B'
  }
];
var detection = [];
var adsbTargets = {};
var selectedAdsbTarget = 'all';

function getAdsbTargetFilterValue() {
  return document.querySelector('#adsb-target-filter')?.value.trim().toLowerCase() || '';
}

function formatAdsbLabel(targetId, targetInfo) {
  return targetInfo.flight ? targetInfo.flight + ' (' + targetId + ')' : targetId;
}

function normalizeAdsbTarget(target, targetId) {
  if (!target) {
    return {delay: [], doppler: [], flight: []};
  }
  var delay = Array.isArray(target.delay) ? target.delay : target.delay !== undefined ? [target.delay] : [];
  var doppler = Array.isArray(target.doppler) ? target.doppler : target.doppler !== undefined ? [target.doppler] : [];
  var flightLabel = target.flight || targetId;
  var length = Math.min(delay.length, doppler.length);
  delay = delay.slice(0, length);
  doppler = doppler.slice(0, length);
  var flight = Array(length).fill(flightLabel);
  return {delay: delay, doppler: doppler, flight: flight};
}

function getAllAdsbTraceSelection() {
  var delay = [];
  var doppler = [];
  var flight = [];
  Object.keys(adsbTargets).sort().forEach(function (targetId) {
    var normalized = normalizeAdsbTarget(adsbTargets[targetId], targetId);
    delay.push.apply(delay, normalized.delay);
    doppler.push.apply(doppler, normalized.doppler);
    flight.push.apply(flight, normalized.flight);
  });
  return {delay: delay, doppler: doppler, flight: flight};
}

function getSelectedAdsbTraceSelection() {
  if (selectedAdsbTarget === 'all' || !adsbTargets[selectedAdsbTarget]) {
    return {delay: [], doppler: [], flight: []};
  }
  return normalizeAdsbTarget(adsbTargets[selectedAdsbTarget], selectedAdsbTarget);
}

function updateAdsbTargetTable() {
  var tableBody = document.querySelector('#adsb-target-table tbody');
  if (!tableBody) {
    return;
  }

  var rows = [];
  var filterValue = getAdsbTargetFilterValue();
  var hasFilter = filterValue.length > 0;
  Object.keys(adsbTargets).sort().forEach(function (targetId) {
    var target = adsbTargets[targetId];
    var flight = target.flight || '';
    var filterText = (flight + ' ' + targetId).toLowerCase();
    if (hasFilter && filterText.indexOf(filterValue) === -1) {
      return;
    }
    var row = document.createElement('tr');
    if (selectedAdsbTarget === targetId) {
      row.classList.add('table-active');
    }
    row.dataset.targetId = targetId;
    row.innerHTML = '<td>' + (flight || '-') + '</td>' +
                    '<td>' + targetId + '</td>';
    row.addEventListener('click', function () {
      selectedAdsbTarget = this.dataset.targetId;
      updateAdsbTargetTable();
      updateAdsbTrace();
    });
    rows.push(row);
  });

  tableBody.innerHTML = '';
  if (Object.keys(adsbTargets).length === 0) {
    var emptyRow = document.createElement('tr');
    var emptyCell = document.createElement('td');
    emptyCell.setAttribute('colspan', '2');
    emptyCell.className = 'text-center text-muted small';
    emptyCell.textContent = 'No ADS-B targets available';
    emptyRow.appendChild(emptyCell);
    tableBody.appendChild(emptyRow);
    return;
  }

  var allRow = document.createElement('tr');
  if (selectedAdsbTarget === 'all') {
    allRow.classList.add('table-active');
  }
  allRow.dataset.targetId = 'all';
  allRow.innerHTML = '<td>All ADS-B targets</td><td>-</td>';
  allRow.addEventListener('click', function () {
    selectedAdsbTarget = 'all';
    updateAdsbTargetTable();
    updateAdsbTrace();
  });
  tableBody.appendChild(allRow);
  if (rows.length === 0) {
    var emptyRow = document.createElement('tr');
    var emptyCell = document.createElement('td');
    emptyCell.setAttribute('colspan', '2');
    emptyCell.className = 'text-center text-muted small';
    emptyCell.textContent = hasFilter ? 'No ADS-B targets match the filter' : 'No ADS-B targets available';
    emptyRow.appendChild(emptyCell);
    tableBody.appendChild(emptyRow);
    return;
  }
  rows.forEach(function (row) { tableBody.appendChild(row); });
}

function updateAdsbTrace() {
  var allSelection = getAllAdsbTraceSelection();
  var selectedSelection = getSelectedAdsbTraceSelection();
  Plotly.update('data', {
    x: [allSelection.delay, selectedSelection.delay],
    y: [allSelection.doppler, selectedSelection.doppler],
    text: [allSelection.flight, selectedSelection.flight]
  }, {}, [2, 3]);
}

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
        if (isTruth) {
          $.getJSON(urlAdsb, function () { })
            .done(function (data_adsb) {
              adsbTargets = {};
              for (const aircraft in data_adsb) {
                if ('doppler' in data_adsb[aircraft]) {
                  adsbTargets[aircraft] = {
                    delay: data_adsb[aircraft]['delay'],
                    doppler: data_adsb[aircraft]['doppler'],
                    flight: data_adsb[aircraft]['flight']
                  };
                }
              }
              if (selectedAdsbTarget !== 'all' && !adsbTargets[selectedAdsbTarget]) {
                selectedAdsbTarget = 'all';
              }
              updateAdsbTargetTable();
              updateAdsbTrace();
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

              var allSelection = getAllAdsbTraceSelection();
              var selectedSelection = getSelectedAdsbTraceSelection();
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
              var detectionText = Array.isArray(detection.delay) ? Array(detection.delay.length).fill('Blah2 Target') : [];
              var trace2 = {
                  x: detection.delay,
                  y: detection.doppler,
                  text: detectionText,
                  mode: 'markers',
                  type: 'scatter',
                  marker: {
                    size: 16,
                    opacity: 0.8,
                    color: 'rgba(255, 165, 0, 0.8)'
                  },
                  hovertemplate: 'Range: %{x}<br>Doppler: %{y}',
                  name: 'Blah2 Target'
              };
              var trace3 = {
                x: allSelection.delay,
                y: allSelection.doppler,
                text: allSelection.flight,
                mode: 'markers',
                type: 'scatter',
                marker: {
                  size: 14,
                  opacity: 0.6,
                  color: 'rgba(0, 200, 0, 0.8)'
                },
                hovertemplate: 'ADS-B Target: %{text}<br>Range: %{x}<br>Doppler: %{y}',
                name: 'ADS-B'
              };
              var trace4 = {
                x: selectedSelection.delay,
                y: selectedSelection.doppler,
                text: selectedSelection.flight,
                mode: 'markers',
                type: 'scatter',
                marker: {
                  size: 18,
                  opacity: 1.0,
                  color: 'rgba(255, 255, 0, 1)'
                },
                hovertemplate: 'ADS-B Target:%{text}<br>Range: %{x}<br>Doppler: %{y}',
                name: 'Selected ADS-B'
              };

              var data_trace = [trace1, trace2, trace3, trace4];
              Plotly.newPlot('data', data_trace, layout, config);
            }
            // case update plot
            else {
              var allSelection = getAllAdsbTraceSelection();
              var selectedSelection = getSelectedAdsbTraceSelection();
              var detectionText = Array.isArray(detection.delay) ? Array(detection.delay.length).fill('Blah2 Target') : [];
              var trace_update = {
                x: [data.delay, detection.delay, allSelection.delay, selectedSelection.delay],
                y: [data.doppler, detection.doppler, allSelection.doppler, selectedSelection.doppler],
                z: [data.data, [], [], []],
                zmax: [Math.max(13, data.maxPower), [], [], []],
                text: [[], detectionText, allSelection.flight, selectedSelection.flight]
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
