#! /usr/bin/env bash

echo "MUST BE RUN FROM BINARY DIR";

echo " -> Run Slim";
sudo ./printing_station/display-manager/slim -nodaemon DEV

SUCC=$?

echo "Return Code of slim: $SUCC"

killall Xephyr;
