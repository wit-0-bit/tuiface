// Host-side tests for the PebbleKit JS half (src/pkjs/index.js), run with
// plain `node test/test_pkjs.js`. The script is evaluated in a vm sandbox
// with Pebble, XHR, geolocation, localStorage and setTimeout mocked so the
// retry and caching behavior can be driven deterministically.
var fs = require('fs');
var path = require('path');
var vm = require('vm');

var SRC = fs.readFileSync(path.join(__dirname, '..', 'src', 'pkjs', 'index.js'), 'utf8');

var failures = 0;
function assert(cond, msg) {
  if (cond) {
    console.log('PASS: ' + msg);
  } else {
    failures++;
    console.error('FAIL: ' + msg);
  }
}

// Builds a fresh sandbox, evaluates index.js in it, and returns handles to
// the mocks. geoMode: 'success' | 'fail' (switchable per-test via env.geoMode).
function makeEnv(opts) {
  opts = opts || {};
  var env = {
    timers: [],  // captured setTimeout calls: {fn, ms}
    xhrs: [],    // FakeXHR instances in creation order
    sent: [],    // dicts passed to Pebble.sendAppMessage
    geoCalls: 0,
    geoMode: opts.geoMode || 'success',
    listeners: {},
    store: opts.store || {},
  };

  function FakeXHR() { env.xhrs.push(this); }
  FakeXHR.prototype.open = function(method, url) { this.url = url; };
  FakeXHR.prototype.send = function() {};

  var sandbox = {
    console: {log: function() {}},
    Date: Date,
    JSON: JSON,
    Math: Math,
    setTimeout: function(fn, ms) { env.timers.push({fn: fn, ms: ms}); },
    XMLHttpRequest: FakeXHR,
    navigator: {
      geolocation: {
        getCurrentPosition: function(onSuccess, onError) {
          env.geoCalls++;
          if (env.geoMode === 'success') {
            onSuccess({coords: {latitude: 37.77, longitude: -122.42}});
          } else {
            onError({message: 'denied'});
          }
        }
      }
    },
    localStorage: {
      getItem: function(k) { return (k in env.store) ? env.store[k] : null; },
      setItem: function(k, v) { env.store[k] = String(v); }
    },
    Pebble: {
      addEventListener: function(name, fn) { env.listeners[name] = fn; },
      sendAppMessage: function(dict, onSuccess, onError) {
        env.sent.push(dict);
        if (onSuccess) onSuccess({});
      }
    },
    require: function(name) {
      if (name === '@rebble/clay') return function Clay() {};
      return {};  // ./config.json — contents irrelevant here
    }
  };
  vm.createContext(sandbox);
  vm.runInContext(SRC, sandbox);

  env.fire = function(name) { env.listeners[name]({}); };
  env.runTimer = function() {
    var t = env.timers.shift();
    t.fn();
  };
  return env;
}

function weatherBody() {
  var hours = [];
  var uv = [];
  for (var i = 0; i < 12; i++) {
    hours.push(new Date(Date.now() + i * 3600 * 1000).toISOString());
    uv.push(3 + (i % 3));  // peaks at 5
  }
  return JSON.stringify(
      {current_weather: {temperature: 71.6, weathercode: 0}, hourly: {time: hours, uv_index: uv}});
}

// `this` must be the xhr: index.js reads this.responseText in onload.
function respond(xhr, status, body) {
  xhr.status = status;
  xhr.responseText = body;
  xhr.onload.call(xhr);
}

// --- ready with no cache fetches via geolocation ---
(function() {
var env = makeEnv();
env.fire('ready');
assert(env.geoCalls === 1, 'ready with empty cache triggers a fetch');
})();

// --- geolocation failure retries twice, then gives up ---
(function() {
var env = makeEnv({geoMode: 'fail'});
env.fire('ready');
assert(env.timers.length === 1, 'geolocation failure schedules a retry');
assert(env.timers[0].ms === 15000, 'retry delay is 15s');
env.runTimer();
assert(env.geoCalls === 2 && env.timers.length === 1, 'first retry runs and reschedules');
env.runTimer();
assert(env.geoCalls === 3 && env.timers.length === 0, 'retries exhausted after two attempts');

// A new request from the watch starts with a fresh retry budget
env.fire('appmessage');
assert(env.geoCalls === 4 && env.timers.length === 1, 'watch request resets the retry budget');
})();

// --- each weather failure mode schedules a retry ---
(function() {
var env = makeEnv();
env.fire('ready');
respond(env.xhrs[0], 500, '');
assert(env.timers.length === 1, 'HTTP error status schedules a retry');

env = makeEnv();
env.fire('ready');
env.xhrs[0].onerror();
assert(env.timers.length === 1, 'network error schedules a retry');

env = makeEnv();
env.fire('ready');
env.xhrs[0].ontimeout();
assert(env.timers.length === 1, 'timeout schedules a retry');

env = makeEnv();
env.fire('ready');
respond(env.xhrs[0], 200, 'not json');
assert(env.timers.length === 1, 'unparseable body schedules a retry');
})();

// --- success sends a stamped payload and caches it ---
(function() {
var env = makeEnv();
var before = Math.round(Date.now() / 1000);
env.fire('ready');
respond(env.xhrs[0], 200, weatherBody());
respond(env.xhrs[1], 200, JSON.stringify({current: {us_aqi: 42}}));

assert(env.sent.length === 1, 'successful fetch sends one message');
var dict = env.sent[0];
assert(dict['WEATHER_TEMP'] === 72, 'temperature is rounded');
assert(dict['WEATHER_COND'] === 'SUN', 'weathercode 0 maps to SUN');
assert(dict['WEATHER_AQI'] === 42, 'AQI is included');
assert(dict['WEATHER_UV'] === 5, 'UV peak over the window is included');
assert(
    dict['WEATHER_FETCHED_AT'] >= before && dict['WEATHER_FETCHED_AT'] <= before + 5,
    'payload is stamped with the fetch time in seconds');
assert(env.timers.length === 0, 'no retry pending after success');

var cache = JSON.parse(env.store['weather-cache']);
assert(
    cache && cache.payload && cache.payload['WEATHER_TEMP'] === 72,
    'payload is cached in localStorage');
})();

// --- AQI failure degrades gracefully, still stamped ---
(function() {
var env = makeEnv();
env.fire('ready');
respond(env.xhrs[0], 200, weatherBody());
env.xhrs[1].onerror();

assert(
    env.sent.length === 1 && env.sent[0]['WEATHER_AQI'] === -1,
    'AQI failure still sends weather with AQI sentinel');
assert(env.sent[0]['WEATHER_FETCHED_AT'] > 0, 'degraded payload is still stamped');
assert(env.timers.length === 0, 'AQI failure does not trigger a weather retry');
})();

// --- fresh cache on relaunch is resent without fetching, keeping its stamp ---
(function() {
var fetchedAtSec = Math.round(Date.now() / 1000) - 600;
var cached = {
  payload: {
    'WEATHER_TEMP': 65,
    'WEATHER_COND': 'CLD',
    'WEATHER_AQI': 30,
    'WEATHER_UV': 2,
    'WEATHER_FETCHED_AT': fetchedAtSec
  },
  fetchedAt: Date.now() - 600 * 1000
};
var env = makeEnv({store: {'weather-cache': JSON.stringify(cached)}});
env.fire('ready');

assert(env.geoCalls === 0, 'fresh cache suppresses the network fetch');
assert(env.sent.length === 1 && env.sent[0]['WEATHER_TEMP'] === 65, 'cached payload is resent');
assert(
    env.sent[0]['WEATHER_FETCHED_AT'] === fetchedAtSec,
    'resent payload keeps its original fetch stamp');
})();

// --- a stale cache is ignored and a fetch happens instead ---
(function() {
var cached = {payload: {'WEATHER_TEMP': 65}, fetchedAt: Date.now() - 31 * 60 * 1000};
var env = makeEnv({store: {'weather-cache': JSON.stringify(cached)}});
env.fire('ready');
assert(env.geoCalls === 1 && env.sent.length === 0, 'stale cache falls through to a fetch');
})();

// --- failure then success: the retry delivers ---
(function() {
var env = makeEnv({geoMode: 'fail'});
env.fire('ready');
env.geoMode = 'success';
env.runTimer();
respond(env.xhrs[0], 200, weatherBody());
respond(env.xhrs[1], 200, JSON.stringify({current: {us_aqi: 10}}));
assert(env.sent.length === 1, 'retry after a transient failure delivers weather');
})();

if (failures > 0) {
  console.error(failures + ' assertion(s) failed');
  process.exit(1);
}
console.log('All pkjs tests passed');
