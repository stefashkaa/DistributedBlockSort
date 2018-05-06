#!/bin/bash

ver=18
if [[ -n "$1" && "$1" -eq "16" ]]
then
ver=$1
fi
echo  "┌──────────────────────────────────────────────────────────────┐"
echo  "│                         ICPC ver. $ver                         │"
echo  "└──────────────────────────────────────────────────────────────┘"

mkdir ./build
if [ "$ver" -eq "18" ]
then
source /opt/intel/bin/compilervars.sh intel64
which icpc /opt/intel/bin/icpc
export LD_LIBRARY_PATH=/opt/intel/compilers_and_libraries_2018.2.199/linux/compiler/lib/intel64_lin/:$LD_LIBRARY_PATH
else
source /home/stefan/intel16/bin/compilervars.sh intel64
which icpc /home/stefan/intel16/bin/icpc
export LD_LIBRARY_PATH=/home/stefan/intel16/compilers_and_libraries_2016.3.210/linux/compiler/lib/intel64_lin/:$LD_LIBRARY_PATH
fi
icpc -qopenmp -Wall -I./include/ -L./lib/ -obuild/blocksort main.cpp -lcurl
exit 0