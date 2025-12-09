const SPEED_OF_LIGHT = 299792458;
const KNOTS_TO_MS = 0.514444;
const FTMIN_TO_MS = 0.00508;

function lla2ecef(latitude, longitude, altitude) {
  const radian = Math.PI / 180.0;
  const a = 6378137.0;
  const f = 1.0 / 298.257223563;
  const b = (1.0 - f) * a;
  const esq = 2.0 * f - f * f;
  const latRad = latitude * radian;
  const lonRad = longitude * radian;
  const N = a / Math.sqrt(1.0 - esq * Math.sin(latRad) * Math.sin(latRad));
  const x = (N + altitude) * Math.cos(latRad) * Math.cos(lonRad);
  const y = (N + altitude) * Math.cos(latRad) * Math.sin(lonRad);
  const z = (N * (1.0 - esq) + altitude) * Math.sin(latRad);
  return { x, y, z };
}

function norm(vector) {
  if (Array.isArray(vector)) {
    return Math.sqrt(vector.reduce((sum, value) => sum + value ** 2, 0));
  }
  return Math.sqrt(vector.x ** 2 + vector.y ** 2 + vector.z ** 2);
}

function ft2m(feet) {
  return feet * 0.3048;
}

function enuToEcef(vel_e, vel_n, vel_u, lat_rad, lon_rad) {
  const sin_lat = Math.sin(lat_rad);
  const cos_lat = Math.cos(lat_rad);
  const sin_lon = Math.sin(lon_rad);
  const cos_lon = Math.cos(lon_rad);
  const vx = -sin_lon * vel_e - sin_lat * cos_lon * vel_n + cos_lat * cos_lon * vel_u;
  const vy =  cos_lon * vel_e - sin_lat * sin_lon * vel_n + cos_lat * sin_lon * vel_u;
  const vz =  cos_lat * vel_n + sin_lat * vel_u;
  return {x: vx, y: vy, z: vz};
}

function computeBistaticDelay(aircraft, rx, tx) {
  if (!aircraft.lat || !aircraft.lon || !aircraft.alt_baro) {
    return null;
  }
  const alt_m = ft2m(aircraft.alt_baro);
  const ecefAircraft = lla2ecef(aircraft.lat, aircraft.lon, alt_m);
  const ecefRx = lla2ecef(rx.latitude, rx.longitude, rx.altitude);
  const ecefTx = lla2ecef(tx.latitude, tx.longitude, tx.altitude);
  const dRxTar = norm([
    ecefRx.x - ecefAircraft.x,
    ecefRx.y - ecefAircraft.y,
    ecefRx.z - ecefAircraft.z
  ]);
  const dTxTar = norm([
    ecefTx.x - ecefAircraft.x,
    ecefTx.y - ecefAircraft.y,
    ecefTx.z - ecefAircraft.z
  ]);
  const dRxTx = norm([
    ecefRx.x - ecefTx.x,
    ecefRx.y - ecefTx.y,
    ecefRx.z - ecefTx.z
  ]);
  const bistatic_range = dRxTar + dTxTar - dRxTx;
  return bistatic_range / 1000;
}

function computeBistaticDoppler(aircraft, rx, tx, fc) {
  if (!aircraft.lat || !aircraft.lon || !aircraft.alt_baro || !aircraft.gs || !aircraft.track) {
    return null;
  }
  const alt_m = ft2m(aircraft.alt_baro);
  const ecefAircraft = lla2ecef(aircraft.lat, aircraft.lon, alt_m);
  const ecefRx = lla2ecef(rx.latitude, rx.longitude, rx.altitude);
  const ecefTx = lla2ecef(tx.latitude, tx.longitude, tx.altitude);
  const dRxTar = norm([
    ecefRx.x - ecefAircraft.x,
    ecefRx.y - ecefAircraft.y,
    ecefRx.z - ecefAircraft.z
  ]);
  const dTxTar = norm([
    ecefTx.x - ecefAircraft.x,
    ecefTx.y - ecefAircraft.y,
    ecefTx.z - ecefAircraft.z
  ]);
  if (dRxTar < 100 || dTxTar < 100) {
    return null;
  }
  const gs_ms = aircraft.gs * KNOTS_TO_MS;
  const track_rad = aircraft.track * Math.PI / 180;
  const vel_east = gs_ms * Math.sin(track_rad);
  const vel_north = gs_ms * Math.cos(track_rad);
  let vel_up = 0;
  if (aircraft.geom_rate !== undefined && !isNaN(aircraft.geom_rate)) {
    vel_up = aircraft.geom_rate * FTMIN_TO_MS;
  }
  const lat_rad = aircraft.lat * Math.PI / 180;
  const lon_rad = aircraft.lon * Math.PI / 180;
  const vel_ecef = enuToEcef(vel_east, vel_north, vel_up, lat_rad, lon_rad);
  const vec_to_rx = {
    x: (ecefRx.x - ecefAircraft.x) / dRxTar,
    y: (ecefRx.y - ecefAircraft.y) / dRxTar,
    z: (ecefRx.z - ecefAircraft.z) / dRxTar
  };
  const vec_to_tx = {
    x: (ecefTx.x - ecefAircraft.x) / dTxTar,
    y: (ecefTx.y - ecefAircraft.y) / dTxTar,
    z: (ecefTx.z - ecefAircraft.z) / dTxTar
  };
  const range_rate_rx = -(vel_ecef.x * vec_to_rx.x + vel_ecef.y * vec_to_rx.y + vel_ecef.z * vec_to_rx.z);
  const range_rate_tx = -(vel_ecef.x * vec_to_tx.x + vel_ecef.y * vec_to_tx.y + vel_ecef.z * vec_to_tx.z);
  const bistatic_range_rate = range_rate_rx + range_rate_tx;
  const wavelength = SPEED_OF_LIGHT / fc;
  const doppler = -bistatic_range_rate / wavelength;
  return doppler;
}

module.exports = {
  computeBistaticDelay,
  computeBistaticDoppler,
  lla2ecef,
  norm,
  ft2m
};
