#!/usr/bin/perl -Tw
#
#  PROGRAM:  redirect.cgi
#
#  PURPOSE:  A simple Perl redirect CGI example.
#            Demonstrates how to generate a simple redirect request 
#            for the remote browser that accesses this CGI program.
#
#  Created by alvin alexander, devdaily.com.
#

#-----------------------------------#
#  1. Create a new Perl CGI object  #
#-----------------------------------#

use CGI;
$query = new CGI;

#----------------------------------------------------------------------#
#  2. Issue the redirection request.  (Always use an 'absolute' URL.)  #
#----------------------------------------------------------------------#

print $query->redirect('http://localhost:6969/html/redir.html');
