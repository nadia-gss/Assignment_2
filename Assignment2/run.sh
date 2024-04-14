#!/bin/bash

count=500
interval=1

for ((i=1; i<=count; i++))
do
   echo "Runtime : $i "
   ./publish_temp
   
   sleep $interval
done
