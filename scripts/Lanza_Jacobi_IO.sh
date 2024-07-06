nodefile=../run/nodefile1.dat

# Uncomment for a local installation of the libraries
#MPIPATH=~/LIBS/mpich/bin/


controllernode=`cat ../controller/controller.dat`

echo mpiexec -genvall -f $HOME/FlexMPI/controller/rankfiles/rankfile$4 -np $1 $HOME/FlexMPI/examples/jacobi_IO $5 "${10}" 0.00001 $7 $8 $6 $4 $controllernode -cfile ../run/nodefile2.dat -policy-malleability-triggered -lbpolicy-static -ni 20 -ports $2 $3 -controller $controllernode -IOaction $9

mpiexec -genvall -f $HOME/FlexMPI/controller/rankfiles/rankfile$4 -np $1 $HOME/FlexMPI/examples/jacobi_IO $5 "${10}" 0.00001 $7 $8 $6 $4 $controllernode -cfile ../run/nodefile2.dat -policy-malleability-triggered -lbpolicy-static -ni 20 -ports $2 $3 -controller $controllernode -IOaction $9


