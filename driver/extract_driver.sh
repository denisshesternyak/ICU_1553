#!/bin/bash

# Configuration
NETWORK_SHARE="pogo/data1"  # network share
USERNAME="centauri\\atm"  
PASSWORD="a" 
NETWORK_PATH="PRODUCTS/Linux_Kernel_Driver/Source/driver"
LOCAL_DIR="$HOME/excalibur/$1-Setup"
DRIVER_DIR="$LOCAL_DIR/driver"
DRIVER_XATTR="driver_xattr.sh"

if [ -z "$1" ]; then
    echo "Error: Missing argument for protocol name"
    exit 1
fi

# Files to compare
FILES=("excalbr.c" "builddriver" "callendio.c")
# EXISTING_TAR=$(find ~ -type f \( -name "ExcLinuxDriver.tar.gz" -o -name "ExcLinuxDriver.tgz" \))
# Find tarballs but prioritize the valid ones over those in Trash
EXISTING_TAR=$(find ~ -type f \( -name "ExcLinuxDriver.tar.gz" -o -name "ExcLinuxDriver.tgz" \) | grep -v "/.local/share/Trash/" | head -n 1)

echo "Latest kernel driver found: $EXISTING_TAR"

# Function to compare file dates
compare_dates() {
    local network_file="$1"
    local local_file="$2"
    
    local network_date=$(smbclient "//$NETWORK_SHARE" -U "$USERNAME%$PASSWORD" -m SMB3 -c "ls $NETWORK_PATH/$network_file" 2>&1 | grep -oP '\w{3} \w{3} \d{1,2} \d{2}:\d{2}:\d{2} \d{4}' | head -n 1)
    local network_timestamp=$(date -d "$network_date" +%s)

    local local_date=$(stat --format=%y "$local_file" | sed 's/\([0-9]*-[0-9]*-[0-9]* [0-9]*:[0-9]*:[0-9]*\).*/\1/')
    local local_timestamp=$(date -d "$local_date" +%s)
    
    echo "Network date: $network_date ($network_timestamp)"
    echo "Local date: $local_date ($local_timestamp)"
    
    if [[ "$network_timestamp" -gt "$local_timestamp" ]]; then
        echo "Warning: $network_file is newer on the network than the local version."
        echo "Please update the local file before continuing with installation."
        return 1
    fi
    return 0
}

# Check if network drive is accessible and files are available
smbclient "//$NETWORK_SHARE" -U "$USERNAME%$PASSWORD" -m SMB3 -c "ls $NETWORK_PATH" > smbclient_output.txt 2>&1
if [[ $? -ne 0 ]]; then
    echo "Failed to connect to the network share. Please check your network connection or credentials."
    exit 1
fi

# Check if DRIVER_DIR exists and if the files exist locally before comparing
if [ -d "$DRIVER_DIR" ] && [ "$(ls -A $DRIVER_DIR)" ]; then
    echo "Local driver folder is not empty. Proceeding with file comparison..."

    # Compare the files
    for file in "${FILES[@]}"; do
        local_file="$DRIVER_DIR/$file"
        echo "$file vs $local_file"
        if [ -f "$local_file" ]; then
            compare_dates "$file" "$local_file"
            if [[ $? -ne 0 ]]; then
                echo "Driver is out of date. Please update before continuing."
                exit 1
            fi
        else
            echo "Warning: Local file $file does not exist. Skipping comparison for this file."
        fi
    done
else
    echo "Local driver folder is empty or does not exist. Skipping file comparison and proceeding to extraction."
fi

# Check if the tarball exists
if [[ -n "$EXISTING_TAR" ]]; then
    echo "Tarball found: $EXISTING_TAR"
    
    # Extract the tarball
    echo "Extracting tarball to $LOCAL_DIR..."
    tar -xvzf "$EXISTING_TAR" -C "$LOCAL_DIR"
    
    if [[ $? -eq 0 ]]; then
        echo "Extraction successful."
    else
        echo "Extraction failed. Please check the tarball."
        exit 1
    fi
else
    echo "No tarball found to extract."
fi

# Run the builddriver.sh script
echo "Running builddriver..."
cd $DRIVER_DIR
./builddriver

# Run the excload executable with password
echo "Running excload executable driver..."
echo "a" | sudo -S ./excload

# Call the driver_xattr.sh script
echo "Running driver_xattr.sh script..."
# Run the xattr script
if [[ -f "$DRIVER_DIR/$DRIVER_XATTR" ]]; then
    echo "Running xattr script with protocol $1..."
    ./driver_xattr.sh "$1"
else
    echo "xattr script not found in $DRIVER_DIR"
    exit 1
fi

cd ~/

echo "Driver setup completed successfully."
