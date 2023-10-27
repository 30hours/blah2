#!/bin/bash

# Run script with a crontab to automatically restart on error.
# Checks the API to see if data is still being pushed through.

FIRST_CHAR=$(curl -s 127.0.0.1:3000/map | head -c1)
TIMESTAMP=$(curl -s 127.0.0.1:3000/map | head -c23 | tail -c10)
CURR_TIMESTAMP=$(date +%s)
DIFF_TIMESTAMP=$(($CURR_TIMESTAMP-$TIMESTAMP))

if [[ "$FIRST_CHAR" != "{" ]] || [[ $DIFF_TIMESTAMP -gt 60 ]]; then
  docker compose -f /opt/blah2/docker-compose.yml down
  systemctl restart sdrplay.service
  docker compose -f /opt/blah2/docker-compose.yml up -d
  echo "Successfully restarted blah2"
fi
