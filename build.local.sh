mkdir ./build
source /opt/intel/bin/compilervars.sh intel64
which icpc /opt/intel/bin/icpc
export LD_LIBRARY_PATH=/opt/intel/compilers_and_libraries_2018.2.199/linux/compiler/lib/intel64_lin/:$LD_LIBRARY_PATH
icpc -qopenmp -Wall -obuild/blocksort main.cpp