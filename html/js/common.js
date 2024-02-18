function is_localhost(ip) {
  
  if (ip === 'localhost') {
    return true;
  }
  
  const localRanges = ['127.0.0.1', '192.168.0.0/16', '10.0.0.0/8', '172.16.0.0/12'];

  const ipToInt = ip => ip.split('.').reduce((acc, octet) => (acc << 8) + +octet, 0) >>> 0;

  return localRanges.some(range => {
    const [rangeStart, rangeSize = 32] = range.split('/');
    const start = ipToInt(rangeStart);
    const end = (start | (1 << (32 - +rangeSize))) >>> 0;
    return ipToInt(ip) >= start && ipToInt(ip) <= end;
  });

}