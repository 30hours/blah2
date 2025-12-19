var host = window.location.hostname;

$(document).on('keypress', function (e) {
  if (e.which == 32) {
    url = "/capture/toggle";

    $.getJSON('http://' + host + ':3000/capture/toggle', function () { })

      .done(function (data) {
        console.log('API worked');
      })

      .fail(function () {
        console.log('API Fail');
      })

      .always(function () {

      });

  }
});
