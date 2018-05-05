mkdir ./target
chmod a+x run.sh
source /etc/profile.d/modules.sh
export LD_LIBRARY_PATH=/home/COMMON/intel/compilers_and_libraries_2016.3.210/linux/compiler/lib/intel64_lin/:$LD_LIBRARY_PATH
module load intel/icc16
icpc -qopenmp -Wall -I ./include/ -L ./lib/ -obuild/blocksort main.cpp