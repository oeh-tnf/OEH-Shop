#! /usr/bin/env bash

echo "MUST BE RUN FROM BINARY DIR";

echo " -> Run Xephyr";
Xephyr -br -ac -noreset -screen 800x600 :2 &

sleep 1;

echo " -> Run Slim";
sudo DISPLAY=:2 ./printing_station/display-manager/slim -nodaemon DEV

SUCC=$?

echo "Return Code of slim: $SUCC"

killall Xephyr;
