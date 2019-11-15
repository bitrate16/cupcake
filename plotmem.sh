#!/bin/bash

while true; do
ps -C "$0" -o pid=,%mem=,vsz= >> mem.log
gnuplot show_mem.plt
sleep 1
done &