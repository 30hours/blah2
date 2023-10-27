#!/bin/bash

# Run script with a crontab to automatically restart on error.
# Checks the API to see if data is still being pushed through.

FIRST_CHAR=$(curl -s 127.0.0.1:3000/map | head -c1)

if [[ "$FIRST_CHAR" != "{" ]]; then
  docker compose -f /opt/blah2/docker-compose.yml down
  systemctl restart sdrplay.service
  docker compose -f /opt/blah2/docker-compose.yml up -d
  echo "Successfully restarted blah2"
fi
