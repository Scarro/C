#!/bin/bash
# Sergio Carro Albarrán
# GITT Sistemas Operativos
# renword.sh

if [[ $# < 1 ]]
then
    echo "Usage: no arguments."
    exit 1
elif [[ $# > 1 ]]
then
    echo "Usage: only 1 argument required."
    exit 1
fi

palabra=$1
lista=`ls`

for fichero in $lista
    do
        if grep '^'$palabra $fichero 2>/dev/null 1>/dev/null;
        then
            mv $fichero $fichero.$palabra
        fi
    done
exit 0