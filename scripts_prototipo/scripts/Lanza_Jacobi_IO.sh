nodefile=$HOME/shared/FlexMPI/run/nodefile.dat
MPIPATH=/LIBS/mpich/bin/
executable=$HOME/shared/FlexMPI/examples/jacobi_IO
controllernode=`cat ${HOME}/shared/FlexMPI/controller/controller.dat`
rankfile=$HOME/shared/FlexMPI/controller/rankfiles/rankfile1

list=`cat  $nodefile |  awk -F: '{print $1}' `
for item in $list; do
  echo Configuring node $item
  rsh $item rm -f /tmp/jacobi_IO_${USER}_$4
  rsh $item cp $executable /tmp/jacobi_IO_${USER}_$4
done

cp $executable /tmp/jacobi_IO_${USER}_$4

$MPIPATH/mpiexec -genvall -np $1 -f $rankfile /tmp/jacobi_IO_${USER}_$4 $5 "${10}" 0.00001 $7 $8 $6 $4 "${11}" "${12}" "${13}" $controllernode -cfile $nodefile -policy-malleability-triggered -lbpolicy-static -ni 20 -ports $2 $3 -controller $controllernode -IOaction $9 -alloc:0


