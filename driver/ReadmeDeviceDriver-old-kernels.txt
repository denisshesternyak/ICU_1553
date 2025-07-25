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

B] Compilation under kernel version 2.6 and above:

For 2.6.18-8 or higher: You must install the kernel-level source files.

For 2.6.9-34:
1] Install the kernel-level package (found on the Read Hat Enterprise 4 
Disk Set, Disk 3. Note that this is disk 3 of the main disk set, *not* 
the source disk set).
2] In the kernel directory, go to include/asm-i386/mach-default, and 
copy all files found there into the include directory (two levels above)

For 2.6.9-34 and 2.6.9-1.667:
Go to the driver directory and run the following command (making sure to replace the parameter
"/lib/modules..." with the appropriate directory in which the kernel sources are installed):
(For 2.6.9-34.EL) make -C /lib/modules/2.6.9-34.EL/build SUBDIRS=$PWD modules
(For 2.6.9-34.ELsmp) make -C /lib/modules/2.6.9-34.ELsmp/build SUBDIRS=$PWD modules
(For 2.6.9-1.667) make -C /lib/modules/2.6.9-1.667/build SUBDIRS=$PWD modules
(For 2.6.9-1.667smp) make -C /lib/modules/2.6.9-1.667smp/build SUBDIRS=$PWD modules
The resulting binary will be named excalbr.ko.

To compile a module for the currently running kernel, use the builddriver
script in the driver directory.
    
C] Compilation under kernel versions 2.2.x and 2.4.x:
    
For Linux Red Hat Enterprise 3 - Install the kernel-sources package from disc 3 of the installation 
set (note: *not* disc three of the source set).

Go to the driver directory and run gcc as follows (replacing "/lib/modules..." with the appropriate directory
 in which the kernel sources are installed):
gcc -c -O2 -I/lib/modules/2.4.18-3/build/include excalbr.c
(For 2.4.21-40.EL) gcc -c -O2 -I/lib/modul es/2.4.21-40.EL/build/include excalbr.c
(For 2.4.21-40.ELsmp) gcc -c -O2 -I/lib/modules/2.4.21-40.ELsmp/build/include excalbr.c

The resulting binary will be named excalbr.o

Note: In order for the compilation to succeed, the kernel sources must be installed, configured, and prepared.
Contact your Linux distribution manufacturer for details on the installation of the kernel sources.

D] To load/unload the driver:
Go to the driver directory and run the excload script. NOTE: for linux kernels prior to version 2.6, 
the binary is named excalbr.o rather than excalbr.ko. Therefore, if loading on one of these earlier kernels,
edit the excload script such that the initial insmod command targets the file "excalbr.o".
To unload, run excunload.
Note that in order to provide access to the driver to your users, you may wish to adjust the permissions on the 
driver accordingly.
Appropriate chmod and chgrp statements may be added to the excload script as needed.

E]If any problems are encountered in compiling the driver, first download the
latest version of the separate Linux Kernel Driver package from our website.

2] Updating the Excalibur Software Tools package

a] Bugfix: the Excalibur Software Tools packages distributed as of April 2005 contained a bug regarding high memory 
   mapping. To fix this, replace the API source file deviceio.c with the version included here in the phex directory.

b] Adaptations for Kernel 2.6:
The Excalibur Software Tools packages distributed as of April 2005 were prepared to be compiled  with
kernel 2.2.x or 2.4.x. In order to use them with kernel 2.6, please make the following changes:
i]  replace the header file excsysio.h with the version of the file included here in the phex directory. 
ii] in the makefiles included with the Software Tools, the library /usr/lib/libc.a must be added to the end
of the LIBS line.


3] DEBUG OPTIONS

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
