#!/bin/bash
STARTTIME=$(date +%s)
for (( c=1; c<=3000000; c++ ))
do
   ./a.out
done
ENDTIME=$(date +%s)
echo "It takes $(($ENDTIME - $STARTTIME)) seconds to complete this task..."