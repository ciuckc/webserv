#! /bin/zsh

argc=$#
if [ $argc -lt 3 ] || [ $argc -gt 4 ]; then
	echo "Usage: ./get.sh host port resource"
	exit 1
fi
if [[ $3 == "/*" ]]; then
	echo "Resource must be a relative path"
	exit 1
fi

if [[ $4 == "-c" ]]; then
	touch ../$3 && chmod +x ../$3
fi
request="DELETE /$3 HTTP/1.1
Host: $1:$2
Connection: close
"

echo $request | netcat $1 $2
exit $?
