#!/bin/bash
num=1
if [[ $#=1 ]]
    then
        num=$1
elif [[ $# > 1 ]]
    then
        echo "Usage: [num of archives]."
        exit 1
fi

ls -Rl | grep '^-' | sort -nrk 5 | head -n $num | awk '{ print $9 " " $5 }'