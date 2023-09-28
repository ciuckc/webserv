#!/usr/bin/python3
import os
print ("Content-Type: text/html\r\n\r\n")
for param in os.environ.keys():
   print ("<b>%20s</b>: %s</br>" % (param, os.environ[param]))
print()
