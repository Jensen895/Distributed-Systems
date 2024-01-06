#!/bin/bash

# Check if at least 3 arguments are provided
if [ $# -lt 3 ]; then
    echo "Usage: ./mp2_demo <username> <XX> <-p|-j|-l>"
    exit 1
fi

# Extract the username, XX, and option (-p or -j) from the arguments
username="$1"
XX="$2"
option="$3"

# Define the base hostname
base_hostname="fa23-cs425-41$XX.cs.illinois.edu"

# Define the port and command based on the option
if [ "$option" == "-p" ]; then
    port="50$XX"
    command="./mp1/client membership print_list $base_hostname $port"
elif [ "$option" == "-j" ]; then
    port="50$XX"
    command="./mp1/client membership join $base_hostname $port"
elif [ "$option" == "-l" ]; then
    port="50$XX"
    command="./mp1/client membership leave $base_hostname $port"
elif [ "$option" == "-s" ]; then
    port="50$XX"
    command="./mp1/client membership suspicion $base_hostname $port"
else
    echo "Invalid option: $option"ƒƒ
    exit 1
fi

# Construct the SSH command and execute it
ssh_command="ssh $username@$base_hostname \"$command\""
echo "Running: $ssh_command"
eval "$ssh_command"
