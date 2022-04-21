#!/bin/bash
shopt -s extglob

#script will go into the directory in $1 and delete all the files except
#named as makefile, c files or header files.
cd $1
rm -v !(*.c|*.h|Makefile.*|makefile.*|"Makefile"|"makefile")

shopt -u extglob