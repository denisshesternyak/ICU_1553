#!/bin/bash

# Store the original directory
original_dir=$(pwd)

# Check if the first argument is provided
if [ -z "$1" ]; then
    echo ""
    echo "ENSURE this script is executed from the '~/excalibur' folder"
    echo ""
    echo "No protocol folder-name provided."
    echo "Please choose one of the following protocol options:"
    echo "1. 1553"
    echo "2. 429"
    echo "3. Serial"
    echo "4. CAN825"
    echo "Enter your choice (1-4), or press Enter to use the default (1553):"

    read choice
    case $choice in
        1) project="1553" ;;
        2) project="429" ;;
        3) project="Serial" ;;
        4) project="CAN825" ;;
        *) project="1553" ;;  # Default value
    esac
else
    project="$1"
fi

# Define the global driver directory
file="./excalbr.ko"
driverdir="$HOME/excalibur/$project-Setup/driver"


# Change to the driver directory
cd "$driverdir" || { echo "Failed to change directory to $driverdir"; exit 1; }

# Run the excload executable with password
echo "Running excload executable driver..."
echo "a" | sudo -S $driverdir/excload

# Clear all existing extended attributes using getfattr and setfattr
echo "Clearing existing extended attributes for $file..."
setfattr -x user.sha1 "$file"
setfattr -x user.driver "$file"
setfattr -x user.version "$file"
setfattr -x user.kernel "$file"
setfattr -x user.rhel "$file"
setfattr -x user.src "$file"

# Extract Kernel Driver Version from /proc/excalbr
driver_version=$(grep "Kernel Driver Version" /proc/excalbr | awk '{print $NF}')
echo "Kernel Driver Version: $driver_version"

# Get the OS architecture (32-bit or 64-bit)
os_architecture=$(uname -m)

# Determine if it's 32-bit or 64-bit and store the appropriate version suffix
if [[ "$os_architecture" == "x86_64" ]]; then
    architecture_bit="64"
else
    architecture_bit="32"
fi

# Combine driver version with architecture bit (e.g., 2.9.0.64)
full_driver_version="$driver_version.0.$architecture_bit"
echo "Full Kernel Driver Version: $full_driver_version"

# Extract RHEL Version and Kernel Version (from modinfo)
rhel_version=$(modinfo ./excalbr.ko | grep "rhelversion" | awk '{print $NF}')
src_version=$(modinfo ./excalbr.ko | grep "srcversion" | awk '{print $NF}')
kernel_version=$(modinfo ./excalbr.ko | grep -m 1 '^vermagic' | awk '{print $2}')
sha1=$(sha1sum "$file" | awk '{ print $1 }')
echo "RHEL Linux Version: $rhel_version"
echo "Kernel Version: $kernel_version"
echo "Source Version: $src_version"
echo "SHA-1 of Driver: $sha1"

# Update the extended attributes using setfattr
setfattr -n user.sha1 -v "$sha1" "$file"
setfattr -n user.driver -v "$full_driver_version" "$file"
setfattr -n user.version -v "$full_driver_version" "$file"
setfattr -n user.rhel -v "$rhel_version" "$file"
setfattr -n user.src -v "$src_version" "$file"
setfattr -n user.kernel -v "$kernel_version" "$file"

echo "Extended attributes updated for excalbr.ko"

# Print the extended attributes using getfattr
echo "Displaying extended attributes for excalbr.ko:"
getfattr -d "$file"  # The '-d' option prints all extended attributes

# Define the path of the RevisionHistoryXXX.txt file
REVISION_HISTORY_FILE="$driverdir/../RevisionHistory${project}.txt"

# Get the last modified date of the .ko file (formatted)
FORMATTED_NETWORK_TIMESTAMP=$(stat -c %y "$file" | cut -d'.' -f1)

# Format the new lines to insert into the file
LINE_1="$(basename "$file") is now at version $full_driver_version, dated $FORMATTED_NETWORK_TIMESTAMP"
LINE_2="SHA-1 code is $sha1"
LINE_3="RHEL is $rhel_version"
LINE_4="KERNEL is $kernel_version"
LINE_5="SOURCE is $src_version"

# Check if the new entry already exists in the RevisionHistory file
if ! grep -q "$LINE_1" "$REVISION_HISTORY_FILE"; then
    # Insert the new lines into the RevisionHistory file at line 3, shifting down the existing content
    sed -i "3i\\
$LINE_1\\
$LINE_2\\
$LINE_3\\
$LINE_4\\
$LINE_5\\
" "$REVISION_HISTORY_FILE"

    echo "New entry added to $REVISION_HISTORY_FILE"
else
    echo "Entry already exists in $REVISION_HISTORY_FILE. Skipping addition."
fi

# Change back to the original directory
cd "$original_dir" || { echo "Failed to return to the original directory"; exit 1; }

echo "Returned to the original directory: $original_dir"
