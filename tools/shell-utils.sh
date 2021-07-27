#!/bin/bash
function nx_addr2line()
{
    addr=$2
    file=$1
    if [ -z "$addr" -o -z "$file" ];then
        echo "Usage: $FUNCNAME <file> <addr>"
        return 1;
    fi
    addr2line -e $file -Cifsp $addr | awk -F' at|于 ' '{printf "%-50s [ %-s ]\n", $1, $2}'
}
