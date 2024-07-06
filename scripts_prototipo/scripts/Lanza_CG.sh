nodefile=$HOME/shared/FlexMPI/run/nodefile.dat
MPIPATH=/LIBS/mpich/bin/
executable=$HOME/shared/FlexMPI/examples/gradient
controllernode=`cat ${HOME}/shared/FlexMPI/controller/controller.dat`
rankfile=$HOME/shared/FlexMPI/controller/rankfiles/rankfile1

list=`cat  $nodefile |  awk -F: '{print $1}' `
for item in $list; do
  echo Configuring node $item
  rsh $item rm -f /tmp/gradient$4
  rsh $item cp $executable /tmp/gradient$4
done

echo $MPIPATH/mpiexec -genvall -f $HOME/FlexMPI/controller/rankfiles/rankfile$4 -np $1 /tmp/gradient$4 18000 "${10}" 0.00001 $HOME/FlexMPI/examples -cfile ../run/nodefile2.dat  -policy-malleability-triggered -lbpolicy-static -ni 20 -ports $2 $3 -controller $controllernode -IOaction $9

$MPIPATH/mpiexec -genvall -f $rankfile -np $1 /tmp/gradient$4 18000 "${10}" 0.00001 $HOME/shared/FlexMPI/examples -cfile $nodefile -policy-malleability-triggered -lbpolicy-static -ni 20 -ports $2 $3 -controller $controllernode -IOaction $9


