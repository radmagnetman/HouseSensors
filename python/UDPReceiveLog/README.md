Created a python script that is to be run as a debian service.

The script looks at the UDP stack, if data is present it pulls it and formats it to store in a local class. Every minute, the collected data is averaged together and written to a text file. The objects are cleared on file write.

To do:

* Implement a settings file that includes things like where to store the log.

* Visualization system

To make the python script run as a service, I followed directions on this site:

http://blog.scphillips.com/posts/2013/07/getting-a-python-script-to-run-in-the-background-as-a-service-on-boot/
