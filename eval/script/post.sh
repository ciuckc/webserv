#! /bin/zsh

argc=$#
if [ $argc -ne 5 ]; then
	echo "Usage: ./get.sh host port resource arg1 arg2"
	exit 1
fi

body="arg1=$4&arg2=$5"
request="POST $3 HTTP/1.1
Host: $1:$2
Connection: close
Content-Length: ${#body}

$body
"

echo $request | netcat $1 $2
exit $?
