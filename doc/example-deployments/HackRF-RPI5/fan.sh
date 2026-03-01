#!/bin/bash

# Toggle GPIO 17 based on CPU temperature (>60°C)
# Usage: ./fan.sh

# Ensure pinctrl is available
if ! command -v pinctrl &> /dev/null; then
    echo "Error: pinctrl not found. Please install or check your system."
    exit 1
fi

# Set GPIO17 as output (optional – can be omitted if already configured)
sudo pinctrl set 17 op

# Extract the temperature from sensors output
# Expected line: "temp1:        +56.2°C"
temp_raw=$(sensors | grep -A 2 "cpu_thermal-virtual-0" | grep "temp1:" | awk '{print $2}' | tr -d '+°C')

if [ -z "$temp_raw" ]; then
    echo "Error: Could not read temperature from sensors"
    exit 1
fi

# Convert to integer by multiplying by 10 (e.g., 56.2 → 562)
temp_int=$(echo "$temp_raw" | awk '{printf "%d", $1 * 10}')

# Compare with threshold 60.0°C → 600
if [ "$temp_int" -gt 600 ]; then
    sudo pinctrl set 17 dh
    echo "Temperature ${temp_raw}°C is above 60°C → GPIO17 set HIGH"
else
    sudo pinctrl set 17 dl
    echo "Temperature ${temp_raw}°C is ≤60°C → GPIO17 set LOW"		
fi	
