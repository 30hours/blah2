const bistatic = require('./bistatic.js');

function assertAlmostEqual(actual, expected, tolerance, message) {
  const diff = Math.abs(actual - expected);
  if (diff > tolerance) {
    throw new Error(`${message}: expected ${expected}, got ${actual} (diff: ${diff})`);
  }
}

function testLla2Ecef() {
  const rx = {latitude: 37.7644, longitude: -122.3954, altitude: 23};
  const result = bistatic.lla2ecef(rx.latitude, rx.longitude, rx.altitude);
  if (Math.abs(result.x) < 1e6 || Math.abs(result.y) < 1e6 || Math.abs(result.z) < 1e6) {
    throw new Error('ECEF coordinates seem too small');
  }
  console.log(`✓ testLla2Ecef passed (ECEF: ${result.x.toFixed(0)}, ${result.y.toFixed(0)}, ${result.z.toFixed(0)})`);
}

function testFt2m() {
  const result = bistatic.ft2m(5000);
  assertAlmostEqual(result, 1524, 0.1, 'Feet to meters conversion');
  console.log('✓ testFt2m passed');
}

function testNorm() {
  const result1 = bistatic.norm([3, 4, 0]);
  assertAlmostEqual(result1, 5, 0.001, 'Norm of [3,4,0]');
  const result2 = bistatic.norm({x: 3, y: 4, z: 0});
  assertAlmostEqual(result2, 5, 0.001, 'Norm of {x:3, y:4, z:0}');
  console.log('✓ testNorm passed');
}

function testBistaticDelay() {
  const rx = {latitude: 37.7644, longitude: -122.3954, altitude: 23};
  const tx = {latitude: 37.49917, longitude: -121.87222, altitude: 783};
  const aircraft = {
    lat: 37.6,
    lon: -122.1,
    alt_baro: 5000,
    gs: 250,
    track: 45
  };
  const delay = bistatic.computeBistaticDelay(aircraft, rx, tx);
  if (delay === null) {
    throw new Error('Delay calculation returned null');
  }
  if (delay < 0 || delay > 500) {
    throw new Error(`Delay ${delay} km out of expected range [0, 500]`);
  }
  console.log(`✓ testBistaticDelay passed (delay: ${delay.toFixed(2)} km)`);
}

function testBistaticDoppler() {
  const rx = {latitude: 37.7644, longitude: -122.3954, altitude: 23};
  const tx = {latitude: 37.49917, longitude: -121.87222, altitude: 783};
  const fc = 503000000;
  const aircraft = {
    lat: 37.6,
    lon: -122.1,
    alt_baro: 5000,
    gs: 250,
    track: 45
  };
  const doppler = bistatic.computeBistaticDoppler(aircraft, rx, tx, fc);
  if (doppler === null) {
    throw new Error('Doppler calculation returned null');
  }
  if (Math.abs(doppler) > 500) {
    throw new Error(`Doppler ${doppler} Hz out of expected range [-500, 500]`);
  }
  console.log(`✓ testBistaticDoppler passed (doppler: ${doppler.toFixed(2)} Hz)`);
}

function testMissingData() {
  const rx = {latitude: 37.7644, longitude: -122.3954, altitude: 23};
  const tx = {latitude: 37.49917, longitude: -121.87222, altitude: 783};
  const fc = 503000000;
  const aircraft_no_lat = {lon: -122.1, alt_baro: 5000, gs: 250, track: 45};
  const delay1 = bistatic.computeBistaticDelay(aircraft_no_lat, rx, tx);
  if (delay1 !== null) {
    throw new Error('Expected null for aircraft without lat');
  }
  const aircraft_no_gs = {lat: 37.6, lon: -122.1, alt_baro: 5000, track: 45};
  const doppler1 = bistatic.computeBistaticDoppler(aircraft_no_gs, rx, tx, fc);
  if (doppler1 !== null) {
    throw new Error('Expected null for aircraft without ground speed');
  }
  console.log('✓ testMissingData passed');
}

function runTests() {
  console.log('Running bistatic calculation tests...\n');
  try {
    testLla2Ecef();
    testFt2m();
    testNorm();
    testBistaticDelay();
    testBistaticDoppler();
    testMissingData();
    console.log('\nAll tests passed! ✓');
  } catch (e) {
    console.error('\nTest failed:');
    console.error(e.message);
    process.exit(1);
  }
}

runTests();
