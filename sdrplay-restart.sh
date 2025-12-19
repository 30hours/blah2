echo "Restarting SDRplay services..."
# Stop any existing instances of the application
killall -9 blah2 >/dev/null 2>&1 || true

# Restart the SDRplay API service
killall -9 sdrplay_apiService >/dev/null 2>&1 || true
sleep 2

# Optionally restart service if using systemd
# systemctl restart sdrplay.service
# sleep 3

echo "SDRplay environment reset complete"
