log-current
--------------
A simple utility for listing currently active log files in a directory  

Requirements
------------

* (GNU) Make
* ANSI C (C89) Compiler
* Either a POSIX compatible operating system (e.g. Linux, *BSD, OS X) or subsystem (e.g. MinGW, Cygwin)
* (*Optional*) tailf  

Usage
-----
```
log-current [options]

--auto, -a - Automatically select the first log file and ignore all others
--command, -c <command> - Command to be applied to selected log file
--directory, -d <directory> - Set directory to observe
--list, -l - Only list files
--wait, -w <delay> - Wait a specified amount of seconds
```

Building
--------
log-current is built using a Makefile:  
```
make # Build with default settings
make DEFAULT_LOG_DIR=/path/to/your/dir # Build with your own default log directory
make DEFAULT_COMMAND=your-command-goes-here # Build with your own default command
make install # Install log-current
make uninstall # Uninstall log-current
```
TODO
----
* Add recrusive log searching
* Add file extension filter
* Replace tailf with own implementation

License
-------

log-current is licensed under the MIT License. See LICENSE  
