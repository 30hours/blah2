#!/bin/bash

# generate_node_id.sh
# Script to generate unique node ID from RPi5 serial and update config file
# Designed to run at system startup via systemd service

CONFIG_FILE="/opt/blah2/config/config.yml"
BACKUP_SUFFIX=".backup.$(date +%Y%m%d_%H%M%S)"

# Function to extract RPi serial number (last 8 chars)
get_rpi_serial() {
    local serial=$(grep "Serial" /proc/cpuinfo | awk '{print $3}' | tail -c 9)
    if [ -z "$serial" ]; then
        echo "Error: Could not extract RPi serial number" >&2
        exit 1
    fi
    echo "$serial"
}

# Function to check if node_id already exists in config
node_id_exists() {
    grep -q "node_id:" "$CONFIG_FILE"
    return $?
}

# Function to add node_id to config file
add_node_id_to_config() {
    local node_id="$1"
    
    # Create backup only when making changes
    cp "$CONFIG_FILE" "${CONFIG_FILE}${BACKUP_SUFFIX}"
    echo "Config backup created: ${CONFIG_FILE}${BACKUP_SUFFIX}"
    
    # Add node_id after network section
    awk -v node_id="$node_id" '
    /^network:/ { 
        print $0
        network_section = 1
        next 
    }
    network_section && /^[a-zA-Z]/ && !/^  / { 
        print "  node_id: \"" node_id "\""
        print ""
        network_section = 0
    }
    { print }
    END {
        if (network_section) {
            print "  node_id: \"" node_id "\""
        }
    }' "$CONFIG_FILE" > "${CONFIG_FILE}.tmp" && mv "${CONFIG_FILE}.tmp" "$CONFIG_FILE"
}

# Function to update existing node_id (only if different)
update_node_id_in_config() {
    local node_id="$1"
    local current_node_id=$(grep "node_id:" "$CONFIG_FILE" | awk '{print $2}' | tr -d '"')
    
    # Only update if the node_id has changed
    if [ "$current_node_id" != "$node_id" ]; then
        # Create backup only when making changes
        cp "$CONFIG_FILE" "${CONFIG_FILE}${BACKUP_SUFFIX}"
        echo "Config backup created: ${CONFIG_FILE}${BACKUP_SUFFIX}"
        
        # Update existing node_id
        sed -i "s/^  node_id:.*/  node_id: \"$node_id\"/" "$CONFIG_FILE"
        echo "Updated node_id from '$current_node_id' to '$node_id'"
        return 0
    else
        echo "Node_id already correct ($node_id), no update needed"
        return 1
    fi
}

# Main execution
main() {
    echo "Node ID startup check for blah2..."
    
    # Check if config file exists
    if [ ! -f "$CONFIG_FILE" ]; then
        echo "Error: Config file not found: $CONFIG_FILE" >&2
        exit 1
    fi
    
    # Get RPi serial (last 8 chars are already unique)
    echo "Extracting RPi5 serial number..."
    rpi_serial=$(get_rpi_serial)
    echo "RPi5 Serial (last 8): $rpi_serial"
    
    # Generate node ID using the unique serial suffix
    node_id="ret${rpi_serial}"
    echo "Generated Node ID: $node_id"
    
    # Check if node_id already exists and handle accordingly
    if node_id_exists; then
        echo "Node ID exists in config. Checking if update needed..."
        if update_node_id_in_config "$node_id"; then
            echo "Node_id updated in $CONFIG_FILE"
        fi
    else
        echo "Adding new node_id to config..."
        add_node_id_to_config "$node_id"
        echo "Added node_id to $CONFIG_FILE"
    fi
    
    # Verify the change
    echo ""
    echo "Current node_id in config:"
    grep "node_id:" "$CONFIG_FILE" || echo "Warning: node_id not found in config file"
    
    echo ""
    echo "Node ID startup check complete!"
}

# Run main function
main "$@"