const http = require('http');
const bistatic = require('./bistatic.js');

const config = {
  location: {
    rx: {latitude: 37.7644, longitude: -122.3954, altitude: 23},
    tx: {latitude: 37.49917, longitude: -121.87222, altitude: 783}
  },
  capture: {
    fc: 503000000
  },
  truth: {
    adsb: {
      tar1090: 'sfo1.retnode.com',
      delay_tolerance: 2.0,
      doppler_tolerance: 5.0
    }
  }
};

async function fetchADSB() {
  const tar1090_url = `http://${config.truth.adsb.tar1090}/data/aircraft.json`;
  return new Promise((resolve) => {
    http.get(tar1090_url, (resp) => {
      let data = '';
      resp.on('data', (chunk) => { data += chunk; });
      resp.on('end', () => {
        try {
          const json = JSON.parse(data);
          resolve(json.aircraft || []);
        } catch (e) {
          console.error('Error parsing tar1090 response:', e.message);
          resolve([]);
        }
      });
    }).on('error', (err) => {
      console.error('Error fetching from tar1090:', err.message);
      resolve([]);
    });
  });
}

async function testIntegration() {
  console.log('Fetching aircraft from tar1090...');
  const aircraft = await fetchADSB();
  console.log(`Found ${aircraft.length} aircraft\n`);

  if (aircraft.length === 0) {
    console.log('No aircraft data available for testing');
    return;
  }

  const validAircraft = aircraft.filter(ac =>
    ac.lat && ac.lon && ac.alt_baro && ac.gs && ac.track
  );
  console.log(`${validAircraft.length} aircraft have complete position/velocity data\n`);

  console.log('Computing expected delay-Doppler for each aircraft:\n');
  console.log('Hex      Lat       Lon        Alt(ft) Delay(km) Doppler(Hz)');
  console.log('--------------------------------------------------------------');

  for (const ac of validAircraft.slice(0, 10)) {
    const delay = bistatic.computeBistaticDelay(ac, config.location.rx, config.location.tx);
    const doppler = bistatic.computeBistaticDoppler(ac, config.location.rx, config.location.tx, config.capture.fc);

    if (delay !== null && doppler !== null) {
      console.log(
        `${ac.hex} ${ac.lat.toFixed(4).padStart(9)} ${ac.lon.toFixed(4).padStart(10)} ` +
        `${ac.alt_baro.toString().padStart(7)} ${delay.toFixed(2).padStart(9)} ${doppler.toFixed(2).padStart(11)}`
      );
    }
  }

  console.log('\n\nSimulating detection matching:');
  console.log('-------------------------------');

  const testAircraft = validAircraft[0];
  const expected_delay = bistatic.computeBistaticDelay(testAircraft, config.location.rx, config.location.tx);
  const expected_doppler = bistatic.computeBistaticDoppler(testAircraft, config.location.rx, config.location.tx, config.capture.fc);

  const simulated_detection = {
    timestamp: Date.now(),
    delay: [expected_delay + 0.15],
    doppler: [expected_doppler - 0.8],
    snr: [12.5]
  };

  console.log(`\nSimulated detection:`);
  console.log(`  Delay: ${simulated_detection.delay[0].toFixed(2)} km`);
  console.log(`  Doppler: ${simulated_detection.doppler[0].toFixed(2)} Hz`);
  console.log(`  SNR: ${simulated_detection.snr[0].toFixed(1)} dB`);

  console.log(`\nExpected from aircraft ${testAircraft.hex}:`);
  console.log(`  Delay: ${expected_delay.toFixed(2)} km`);
  console.log(`  Doppler: ${expected_doppler.toFixed(2)} Hz`);

  const delay_err = Math.abs(simulated_detection.delay[0] - expected_delay);
  const doppler_err = Math.abs(simulated_detection.doppler[0] - expected_doppler);

  console.log(`\nResiduals:`);
  console.log(`  Delay residual: ${delay_err.toFixed(2)} km (tolerance: ${config.truth.adsb.delay_tolerance} km)`);
  console.log(`  Doppler residual: ${doppler_err.toFixed(2)} Hz (tolerance: ${config.truth.adsb.doppler_tolerance} Hz)`);

  const matched = delay_err < config.truth.adsb.delay_tolerance &&
                  doppler_err < config.truth.adsb.doppler_tolerance;

  console.log(`\nMatch result: ${matched ? '✓ MATCHED' : '✗ NO MATCH'}`);

  if (matched) {
    const result = {
      hex: testAircraft.hex,
      lat: testAircraft.lat,
      lon: testAircraft.lon,
      alt_baro: testAircraft.alt_baro,
      gs: testAircraft.gs,
      track: testAircraft.track,
      expected_delay: Math.round(expected_delay * 100) / 100,
      expected_doppler: Math.round(expected_doppler * 100) / 100,
      delay_residual: Math.round((simulated_detection.delay[0] - expected_delay) * 100) / 100,
      doppler_residual: Math.round((simulated_detection.doppler[0] - expected_doppler) * 100) / 100
    };
    console.log('\nOutput ADS-B association:');
    console.log(JSON.stringify(result, null, 2));
  }
}

testIntegration().catch(console.error);
