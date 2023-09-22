# Webserv configuration file documentation

Description and usage of the parameters in the configuration file.

## server
Creates a new server block. Each server block represents a virtual server. A configuration file can contain multiple server blocks.

### Syntax
```server{...}```

### Arguments
Directives.

# Context
Configuration file.

## listen
Select port to listen on.

### Syntax
```listen <port>```

### Arguments
A valid port number between 49152 and 65535 or 80 or 8080.

### Context
Server block.

## server_name

### Syntax
```server_name <name> ...```

### Arguments
A collection of valid domain names.

### Context
Server block.
