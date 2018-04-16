#!/bin/sh
while :
do
date | awk '{print $5}'
sleep 1
done
