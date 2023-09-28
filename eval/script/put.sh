#!/bin/zsh

if [[ $# -lt 2 ]]; then
	echo "Usage: put.sh <port> <uri> [file]"
	exit 1
elif [[ $# -eq 2 ]]; then
	size=0
else
	size=$(stat -c '%s' ../$3)
fi

port=$1
uri=$2
file=../$3

(cat << eof; [ -z "${file:-}" ] || cat $file) | nc localhost $port
PUT $uri HTTP/1.1
Host: localhost:$port
Connection: close
Content-length: $size

eof
