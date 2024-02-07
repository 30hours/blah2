
var host = window.location.hostname;
var isLocalHost = (host === "localhost" || host === "127.0.0.1" || host === "192.168.0.112");

// setup API
var urlFalseTargets;

if (isLocalHost) {
    urlFalseTargets = '//' + host + ':3000/api/falsetargets';
} else {
    urlFalseTargets = '//' + host + '/api/falsetargets';
}

//callback function
var intervalId = window.setInterval(function () {
    $.getJSON(urlFalseTargets, function (data) {
        if (data != null) {
            var table = document.getElementById("data");


            // PLEASE SOMEONE FORMAT THIS NICER! //
            var output = "";
            data.false_targets.forEach((target) => {
                output += "id: " + target.id + "<br>";
                output += "<ul> <li>" + target.type + "</li>";
                if (target.type === "static") {
                    output += "<li> delay: " + target.delay + "</li>";
                    output += "<li> delay_samples: " + target.delay_samples + "</li>";
                    output += "<li> range: " + target.range + "</li>";
                    output += "<li> doppler: " + target.doppler + "</li>";
                    output += "<li> rcs: " + target.rcs + "</li>";
                } else if (target.type === "moving_radar") {
                    output += "<li> delay: " + target.delay + "</li>";
                    output += "<li> delay_samples: " + target.delay_samples + "</li>";
                    output += "<li> range: " + target.range + "</li>";
                    output += "<li> start_range: " + target.start_range + "</li>";
                    output += "<li> doppler: " + target.doppler + "</li>";
                    output += "<li> doppler_rate: " + target.doppler_rate + "</li>";
                    output += "<li> rcs: " + target.rcs + "</li>";
                }
                output += "</ul>";
            });
            table.innerHTML = output;
            // data.false_targets.foreach((targetjson) => {
            //     target = JSON.parse(targetjson);
            //     console.log(target);
            // });

            // for (var i = 0; i < data.length; i++) {
            //     var row = table.insertRow(i + 1);
            //     var cell1 = row.insertCell(0);
            //     var cell2 = row.insertCell(1);
            //     var cell3 = row.insertCell(2);
            //     cell1.innerHTML = data[i].x;
            //     cell2.innerHTML = data[i].y;
            //     cell3.innerHTML = data[i].z;
            // }
        }
    });
}, 100);