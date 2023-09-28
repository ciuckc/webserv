#! /bin/zsh

# colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

# normal get
response=$(./get.sh localhost 6969 /html/index.html)
if [[ ! $response =~ "200" ]]; then
	echo "$RED [KO] $NC failed normal get test"
else
	echo "$GREEN [OK] $NC passed normal get test"
fi

# get with nonexistent file
response=$(./get.sh localhost 6969 /nonexistent)
if [[ ! $response =~ "404" ]]; then
	echo "$RED [KO] $NC failed get test with nonexistent file"
else
	echo "$GREEN [OK] $NC passed get test with nonexistent file"
fi

# get with illegal method
response=$(./get.sh localhost 6969 /test/file)
if [[ ! $response =~ "405" ]]; then
	echo "$RED [KO] $NC failed get test with illegal method"
else
	echo "$GREEN [OK] $NC passed get test with illegal method"
fi

# get with simple cgi
response=$(./get.sh localhost 6969 /cgi/simple.cgi)
if [[ ! $response =~ "200" ]] || [[ ! $response =~ $cgi_out ]]; then
	echo "$RED [KO] $NC failed get test with simple cgi"
else
	echo "$GREEN [OK] $NC passed get test with simple cgi"
fi

# get with client redir cgi
response=$(./get.sh localhost 6969 /cgi/client_redir.cgi)
if [[ ! $response =~ "302" ]] || [[ ! $response =~ "Location" ]]; then
	echo "$RED [KO] $NC failed get test with client redir cgi"
else
	echo "$GREEN [OK] $NC passed get test with client redir cgi"
fi

# get with server redir cgi
response=$(./get.sh localhost 6969 /cgi/server_redir.cgi)
if [[ ! $response =~ "200" ]]; then
	echo "$RED [KO] $NC failed get test with server redir cgi"
else
	echo "$GREEN [OK] $NC passed get test with server redir cgi"
fi

# get with redir (non-cgi)
response=$(./get.sh localhost 8080 /)
if [[ ! $response =~ "302" ]] || [[ ! $response =~ "Location" ]]; then
	echo "$RED [KO] $NC failed get test with redir"
else
	echo "$GREEN [OK] $NC passed get test with redir"
fi

# post with simple cgi
response=$(./post.sh localhost 6969 /cgi/form.cgi value1 value2)
if [[ ! $response =~ "200" ]] || [[ ! $response =~ value1 ]] || [[ ! $response =~ value2 ]]; then
	echo "$RED [KO] $NC failed post test with simple cgi"
else
	echo "$GREEN [OK] $NC passed post test with simple cgi"
fi

# post with client redir cgi
response=$(./post.sh localhost 6969 /cgi/client_redir_doc.cgi arg1 arg2)
if [[ ! $response =~ "302" ]] || [[ ! $response =~ "Location" ]]; then
	echo "$RED [KO] $NC failed post test with client redir cgi"
else
	echo "$GREEN [OK] $NC passed post test with client redir cgi"
fi

# post with nonexistent file
response=$(./post.sh localhost 6969 /nonexistent.cgi arg1 arg2)
if [[ ! $response =~ "404" ]]; then
	echo "$RED [KO] $NC failed post test with nonexistent file"
else
	echo "$GREEN [OK] $NC passed post test with nonexistent file"
fi

# post with non-cgi
response=$(./post.sh localhost 6969 /test/dummy.cgi arg1 arg2)
if [[ ! $response =~ "405" ]]; then
	echo "$RED [KO] $NC failed post test with non-cgi"
else
	echo "$GREEN [OK] $NC passed post test with non-cgi"
fi

# test cgi not printing rfc-compliant output (including nothing at all)
# test cgi metavariables

# normal delete
response=$(./delete.sh localhost 6969 files/delete_me -c)
if [[ ! $response =~ "204" ]]; then
	echo "$RED [KO] $NC failed delete test with normal file"
else
	echo "$GREEN [OK] $NC passed delete test with normal file"
fi

# delete with nonexistent file
response=$(./delete.sh localhost 6969 files/delete_me)
if [[ ! $response =~ "404" ]]; then
	echo "$RED [KO] $NC failed delete test with nonexistent file"
else
	echo "$GREEN [OK] $NC passed delete test with nonexistent file"
fi

# delete with illegal method
response=$(./delete.sh localhost 6969 html/index.html)
if [[ ! $response =~ "405" ]]; then
	echo "$RED [KO] $NC failed delete test with illegal method"
else
	echo "$GREEN [OK] $NC passed delete test with illegal method"
fi

# test with invalid http version
request="GET /html/index.html HTTP/1.0
Host: localhost:8080
Connection: close
"
response=$(echo $request | netcat localhost 8080)
if [[ ! $response =~ "505" ]]; then
	echo "$RED [KO] $NC failed test with unsupported version"
else
	echo "$GREEN [OK] $NC passed test with unsupported version"
fi

# test with unsupported method
request="HEAD /html/index.html HTTP/1.1
Host: localhost:8080
Connection: close
"
response=$(echo $request | netcat localhost 8080)
if [[ ! $response =~ "405" ]]; then
	echo "$RED [KO] $NC failed test with unsupported method"
else
	echo "$GREEN [OK] $NC passed test with unsupported method"
fi

# test with body_size > max_body_size
request="POST /cgi/form.cgi HTTP/1.1
Host: localhost:6969
Connection: close
content-length: 5000

$(for i in {1..5000}; do echo -n "a"; done)
"
response=$(echo $request | netcat localhost 6969)
if [[ ! $response =~ "413" ]]; then
	echo "$RED [KO] $NC failed test with body_size > max_body_size"
else
	echo "$GREEN [OK] $NC passed test with body_size > max_body_size"
fi

# test with siege
