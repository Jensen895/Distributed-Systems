#!/bin/bash

# Check if at least 3 arguments are provided
if [ $# -lt 4 ]; then
    echo "Usage: ./mp3_multiread username sdfsfile localfile <01-10>"
    exit 1
fi

username="$1"
sdfsfile="$2"
localfile="$3" 
shift 3

for arg in "$@"; do
    # Check if the argument is a valid two-digit number between 01 and 10
    if [[ $arg =~ ^(0[1-9]|10)$ ]]; then
        host="yy63@fa23-cs425-41${arg}.cs.illinois.edu"
        ip="fa23-cs425-41${arg}.cs.illinois.edu"
        echo "Connecting to $ip"
        ssh "$host" "./mp1/client sdfs get $ip 4480 $sdfsfile $localfile "
    else
        echo "Invalid argument: $arg. Should be a two-digit number between 01 and 10."
    fi
done
