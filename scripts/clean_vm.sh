#!/bin/bash

# This script cleans up the logs that garden writes obscene amounts of data to.
# Basically, stops your VM from killing itself. Before I wrote this, my barebones
# VM was taking up over 250GB.
sudo sh -c "echo \"Clearing logs.\"; cat /dev/null &> /var/log/kern.log; cat /dev/null &> /var/log/syslog"
