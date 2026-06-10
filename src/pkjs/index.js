var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig);

Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
  getWeather();
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received!');
  getWeather();
});

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    function(position) {
      var lat = position.coords.latitude;
      var lon = position.coords.longitude;

      // Read units from Clay settings
      var settings = {};
      try {
        settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
      } catch (e) {
        console.log('Error reading clay settings: ' + e);
      }
      var units = settings['SETTINGS_UNITS'] || '0';
      var tempUnit = (units === '1' || units === 1) ? 'celsius' : 'fahrenheit';

      var url = 'https://api.open-meteo.com/v1/forecast?latitude=' + lat + '&longitude=' + lon + '&current_weather=true&temperature_unit=' + tempUnit;

      var xhr = new XMLHttpRequest();
      xhr.onload = function () {
        if (xhr.status === 200) {
          try {
            var json = JSON.parse(this.responseText);
            var temp = Math.round(json.current_weather.temperature);
            var code = json.current_weather.weathercode;
            
            // Map WMO weather codes to simple strings (SUN, CLD, FOG, RAIN, SNOW, TSTM)
            var cond = "SUN";
            if (code === 0) {
              cond = "SUN";
            } else if (code >= 1 && code <= 3) {
              cond = "CLD";
            } else if (code === 45 || code === 48) {
              cond = "FOG";
            } else if ((code >= 51 && code <= 55) || (code >= 61 && code <= 65) || (code >= 80 && code <= 82)) {
              cond = "RAIN";
            } else if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) {
              cond = "SNOW";
            } else if (code >= 95) {
              cond = "TSTM";
            } else {
              cond = "CLD";
            }

            var dict = {
              'WEATHER_TEMP': temp,
              'WEATHER_COND': cond
            };

            Pebble.sendAppMessage(dict,
              function(e) { console.log('Weather sent successfully!'); },
              function(e) { console.log('Error sending weather: ' + JSON.stringify(e)); }
            );
          } catch(e) {
            console.log('Error parsing weather JSON: ' + e);
          }
        } else {
          console.log('Weather HTTP request failed with status: ' + xhr.status);
        }
      };
      xhr.open('GET', url);
      xhr.send();
    },
    function(err) {
      console.log('Error getting location: ' + err.message);
    },
    { timeout: 15000, maximumAge: 60000 }
  );
}
