export LD_LIBRARY_PATH=/home/admin/FlexMPI/lib/:/home/admin/ic/:$LD_LIBRARY_PATH

nodefile=../run/nodefile.dat

# Uncomment for a local installation of the libraries
#MPIPATH=~/LIBS/mpich/bin/


echo ${MPIPATH}mpiexec -genvall -f $HOME/FlexMPI/controller/rankfiles/rankfile -np $1 $HOME/FlexMPI/Fintegration/jacobi 15000 400 0.00001 1 0 0 1 maestro -cfile ../run/nodefile.dat -policy-malleability-triggered -lbpolicy-static -ni 20 -ports 5000 5001 -controller slurm-node-1 -IOaction 1 -alloc:0


mpiexec.mpich -genvall -f $HOME/FlexMPI/controller/rankfiles/rankfile -np $1 $HOME/FlexMPI/Fintegration/jacobi 15000 400 0.00001 1 0 0 1 maestro -cfile ../run/nodefile.dat -policy-malleability-triggered -lbpolicy-static -ni 10 -ports 5000 5001 -controller slurm-node-1 -IOaction 1 -alloc:0


