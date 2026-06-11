var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig);

var WEATHER_CACHE_MAX_AGE_MS = 30 * 60 * 1000;

function sendWeatherDict(dict, logLabel) {
  try {
    localStorage.setItem('weather-cache', JSON.stringify({ payload: dict, fetchedAt: Date.now() }));
  } catch (e) {
    console.log('Error writing weather cache: ' + e);
  }
  Pebble.sendAppMessage(dict,
    function(e) { console.log(logLabel + ' sent successfully!'); },
    function(e) { console.log('Error sending: ' + JSON.stringify(e)); }
  );
}

function readFreshWeatherCache() {
  try {
    var cache = JSON.parse(localStorage.getItem('weather-cache'));
    if (cache && cache.payload && (Date.now() - cache.fetchedAt) < WEATHER_CACHE_MAX_AGE_MS) {
      return cache.payload;
    }
  } catch (e) {
    console.log('Error reading weather cache: ' + e);
  }
  return null;
}

Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
  // The watchface relaunches every time the user navigates back to it;
  // don't hit the network if the last fetch is still fresh. Resend the
  // cached payload so a watch with cleared storage still gets data.
  var cached = readFreshWeatherCache();
  if (cached) {
    console.log('Weather cache fresh, resending cached payload');
    Pebble.sendAppMessage(cached,
      function(e) { console.log('Cached weather sent successfully!'); },
      function(e) { console.log('Error sending: ' + JSON.stringify(e)); }
    );
  } else {
    getWeather();
  }
});

Pebble.addEventListener('appmessage', function(e) {
  // The watch only asks when its own cache is stale — always fetch.
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

      var url = 'https://api.open-meteo.com/v1/forecast?latitude=' + lat + '&longitude=' + lon + '&current_weather=true&timezone=auto&temperature_unit=' + tempUnit + '&daily=uv_index_max';

      var xhr = new XMLHttpRequest();
      xhr.onload = function () {
        if (xhr.status === 200) {
          try {
            var json = JSON.parse(this.responseText);
            var temp = Math.round(json.current_weather.temperature);
            var code = json.current_weather.weathercode;
            // -1 is the watch-side "no data" sentinel (renders as "--")
            var uv = -1;
            if (json.daily && json.daily.uv_index_max && json.daily.uv_index_max.length > 0) {
              uv = Math.round(json.daily.uv_index_max[0]);
            }
            
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

            // Fetch AQI from Air Quality API
            var aqiUrl = 'https://air-quality-api.open-meteo.com/v1/air-quality?latitude=' + lat + '&longitude=' + lon + '&current=us_aqi';
            var aqiXhr = new XMLHttpRequest();
            aqiXhr.onload = function() {
              var aqi = -1;
              if (aqiXhr.status === 200) {
                try {
                  var aqiJson = JSON.parse(this.responseText);
                  if (aqiJson.current && aqiJson.current.us_aqi !== undefined) {
                    aqi = Math.round(aqiJson.current.us_aqi);
                  }
                } catch(e) {
                  console.log('Error parsing AQI: ' + e);
                }
              }
              
              sendWeatherDict({
                'WEATHER_TEMP': temp,
                'WEATHER_COND': cond,
                'WEATHER_AQI': aqi,
                'WEATHER_UV': uv
              }, 'Weather, AQI & UV');
            };
            aqiXhr.onerror = function() {
              sendWeatherDict({
                'WEATHER_TEMP': temp,
                'WEATHER_COND': cond,
                'WEATHER_AQI': -1,
                'WEATHER_UV': uv
              }, 'Weather & UV (no AQI)');
            };
            aqiXhr.ontimeout = aqiXhr.onerror;
            aqiXhr.open('GET', aqiUrl);
            aqiXhr.timeout = 10000;
            aqiXhr.send();

          } catch(e) {
            console.log('Error parsing weather JSON: ' + e);
          }
        } else {
          console.log('Weather HTTP request failed with status: ' + xhr.status);
        }
      };
      xhr.onerror = function() {
        console.log('Weather HTTP request failed (network error)');
      };
      xhr.ontimeout = function() {
        console.log('Weather HTTP request timed out');
      };
      xhr.open('GET', url);
      xhr.timeout = 10000;
      xhr.send();
    },
    function(err) {
      console.log('Error getting location: ' + err.message);
    },
    { timeout: 15000, maximumAge: 60000 }
  );
}
