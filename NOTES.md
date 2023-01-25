# Notes

## Files

### Kernel level
- gwthd.h 	: the api should be place here (created by students)
- proc.c	: where thread code is stored

- syscall.h : define system call constant
- syscall.c : mapping call constant with the function
- sysfile.c : take in arugments from user call. sys to functiom
- defs.h	: define function
- user.h 	: allow user to access system call (optional)
- usys.S	: binding user space function label to the system call table

### User level
- ulib.c	: user functions are stored here, OR
- *.c		: put functions here, but include *.o in ULIB in Makefile
- user.h	: allow user to access system call

## Other notes

- create user stack first
- REMOVE ALL PRINT STATEMENTS BEFORE SUBMITTING!
