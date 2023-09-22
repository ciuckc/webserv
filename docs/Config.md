# Webserv configuration file documentation

Description and usage of the parameters in the configuration file.

## server
Creates a new server block. Each server block represents a virtual server. A configuration file can contain multiple server blocks.

### Syntax
```server{...}```

### Context
Configuration file.

## listen
Select port to listen on.
A valid port number between 49152 and 65535 or 80 or 8080.

### Syntax
```listen <port>;```

### Context
Server block.

## server_name
Sets the name of a virtual server. It can contain multiple names separated by spaces. A name may contain slashes.

### Syntax
```server_name <name> ...;```

### Context
Server block.

## client_max_body_size
Sets the maximum allowed size of the client request body, specified in the “Content-Length” request header field. If the size in a request exceeds the configured value, the 413 (Request Entity Too Large) error is returned to the client.

### Syntax
```client_max_body_size <size>;```

### Context
Server block.

## error_page
Defines the URI to which the request will be redirected if an error occurs.

### Syntax
```error_page <code>... <uri>;```

### Context
Server block.

## location
Sets configuration for a specified URI.

### Syntax
```location <uri> {...}```

### Context
Server block.

## root
Specifies the root directory for requests.

### Syntax
```root <path>;```

### Context
Location block.

## autoindex
Turns directory listing on or off.

### Syntax
```autoindex on | off;```

### Context
Location block.

## index
Sets the file that will be served as a directory index.

### Syntax
```index <file> ...;```

### Context
Location block.

## allowed_methods
Defines the HTTP methods that are allowed for a given location.

### Syntax
```allowed_methods <method> ...;```

### Context
Location block.

## redirect
Sets the a redirection for a given location.

### Syntax
```redirect <uri>;```

### Context
Location block.
