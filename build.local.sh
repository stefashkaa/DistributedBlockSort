mkdir ./build
source /opt/intel/bin/compilervars.sh intel64
which icpc /opt/intel/bin/icpc
export LD_LIBRARY_PATH=/opt/intel/compilers_and_libraries_2018.2.199/linux/compiler/lib/intel64_lin/:$LD_LIBRARY_PATH
icpc -qopenmp -Wall -I ./include/ -L /usr/lib/x86_64-linux-gnu/libcurl-gnutls.so.4.5.0 -obuild/blocksort main.cpp