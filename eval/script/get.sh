#! /bin/zsh

argc=$#
if [ $argc -ne 3 ]; then
	echo "Usage: ./get.sh host port resource"
	exit 1
fi

request="GET $3 HTTP/1.1
Host: $1:$2
Connection: close
"

echo $request | netcat $1 $2
exit $?
