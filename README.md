Authors: Siddhant, Arun, Medha
Net ID: ssm200002, axj200010, mva170001
Course: CS 5348.001
Project 2 Part 2 - V6 FILE SYSTEM



In this project, we built off the modv6 program that we created in Project 2 Part 1 and implemented the additional
following commands: openfs, initfs, cpin, cpout, rm, mkdir, cd, pwd, rootdir and q/exit.
Openfs is used to open/create an existing file. Initfs makes sure that all free blocks are
accessible from free[] array of the super block and that one of the data blocks contains the root
directory’s contents. Cpin will create a new file in the v6 file system and fill the contents of the
newly created file with the contents of the externalfile. The file externalfile is a file in the native unix machine
the system where your program is being run. Cpout will create externalfile and make the externalfile's contents
equal to the file generated in . Rm will delete a file and Remove all the data blocks of the file, free the i-node
and remove the directory entry. Mkdir should create the directory. Cd will change the working directory.
Q/exit will terminate the program.



How to run the code:



Compile this c file: gcc modv6.c -o (name of the generated exe)
Run it with the following format: ./(name of generated exe) (name of file)
Do the following commands
Terminate the program



Work that each person completed:



We all did research and discussed ways to approach this project in addition to debugging.



Siddhant: Worked on the initfs function in addition to getFreeBlock function. In addition he worked on the cpout and mkdir commands.
Arun: Worked on main and readInode and writeInode functions. In addition, he worked on the rm and cd functionality.
Medha: Worked on openfs and addFreeBlock functions. In addition, she worked on the cpin functionality with the quit/exit.
Refactored code to work with linux system calls rather than using c library calls
