#!/bin/bash
echo "start test"
if  test $# -ne 3; then
    echo "param error,please input correct param! for example: ./echo_c 192.168.22.61 9876 1000 \n"
    exit 0
fi
for((i = 0; i < $3; i=i+1))
    do nohup ./echo_c_json $1 $2 &
    sleep 0.1
done
echo "end test"
