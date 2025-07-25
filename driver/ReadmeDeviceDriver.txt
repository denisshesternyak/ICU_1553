Excalibur Device Driver for Linux (PCI boards)

This package contains the most updated device driver for Linux for Excalibur boards.
It may be used to update any available variety of the Excalibur Software Tools for Linux, as described below.

1] The Device Driver
  
The driver directory of this package contains the files required to compile and install 
the device driver for the Excalibur board.  

The files found there are: 

Makefile  - makefile for the device driver

excload   - shell script to dynamically load driver and create node if necessary
excunload - shell script to dynamically unload driver

excalbr.c - the source of the driver
excsysio.h, excalbr.h, exctypes.h - header files for the driver
excalbr.ko - a precompiled device driver binary, compiled for kernel 2.6.18-164.el5

The name of the binary mentioned in the excload file should be either excalbr.ko or excalbr.o accordingly.
For compiling your own kernel version see the instructions immediately below.

To compile the driver:

A] Configuring the driver:
At the top of the excalbr.c file, a define statement determines the device supported by the compile. 
For the EXC-4000PCI board, regardless of the specific modules contained within, use the EXC_BOARD_4000PCI define.
For other boards, choose the appropriate define.

B] Compilation under kernel version 2.6.18-8 and above:

To compile a module for the currently running kernel, use the builddriver
script in the driver directory.
    
The resulting binary will be named excalbr.ko.

Note: In order for the compilation to succeed, the kernel sources must be installed, configured, and prepared.
Contact your Linux distribution manufacturer for details on the installation of the kernel sources.

C] To load/unload the driver:
Go to the driver directory and run the excload script. NOTE: for linux kernels prior to version 2.6, 
the binary is named excalbr.o rather than excalbr.ko. Therefore, if loading on one of these earlier kernels,
edit the excload script such that the initial insmod command targets the file "excalbr.o".
To unload, run excunload.
Note that in order to provide access to the driver to your users, you may wish to adjust the permissions on the 
driver accordingly.
Appropriate chmod and chgrp statements may be added to the excload script as needed.

E]If any problems are encountered in compiling the driver, first download the
latest version of the separate Linux Kernel Driver package from our website.

For old Linux kernels (< 3.0), consult the file ReadmeDeviceDriver-old-kernels.txt in this directory

2] DEBUG OPTIONS

A] Debug Mode

If the programs fail to make contact with the Excalibur board, it is instructive to generate a log file from the 
device driver. Near the top of the excalbr.c file, in the /driver directory, the function DebugMessage is defined 
as an empty function. Redefine this identifier as print and recompile the driver (as described above). 
Unload the previous driver, and load the new one.  A certain amount of debug output will be produced on driver load,
and an additional amount will be produced when one of the demo programs is run. 

The debug output can be found in the file /var/log/messages. All of the driver's debug messages begin EXC; 
thus, a full debug log from the Excalibur driver can be achieved with:
cat /var/log/messages | grep "EXC"

Sending this log file to our tech support will enable us to quickly solve any problems which may have arisen.

B] Phex Utility

The included Phex utility scans the onboard memory of the Excalibur board and provides an output file containing data
of interest.  If errors are encountered while using the Excalibur board, it will be instructive to generate these
phex output files and to send them to Excalibur's tech support staff.

In general, it will be most helpful if the phex output is generated and saved both upon bootup of the linux system,
as well as after running the problematic application.

Additionally, if relevant, the phex utility can be run simultaneously with the problematic application, 
as a parallel task, to provide us with a real time view of the system's memory during execution.

The phex directory includes the phex executable, as well as the header, source, and make files needed to recompile
the utility.

As of kernel driver version 2.9, the driver prints its version to the system
log on loading (if DebugMessage is enabled; see above) and it is also
available from the /proc/excalbr pseudo-file along with much more useful
information.

It is also possible to view information about the kernel driver file
(excalbr.ko) using the modinfo command with the full path to the excalbr.ko
file as its argument. One part of this field is the 'srcversion' and this can
be viewed in a running driver with the command:

cat /sys/module/excalbr/srcversion
