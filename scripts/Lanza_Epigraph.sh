nodefile=../run/nodefile1.dat
MPIPATH=/usr/bin

# Uncomment for a local installation of the libraries
#MPIPATH=~/LIBS/mpich/bin/

list=`cat  $nodefile |  awk -F: '{print $1}' `
for item in $list; do
  echo Configuring node $item
  rsh $item rm -f /tmp/epiGraph$4 
  rsh $item cp $HOME/FlexMPI/examples/EpiGraphFlexMPI/epiGraph /tmp/epiGraph$4
done

controllernode=`cat ../controller/controller.dat`

cd /tmp

echo mpiexec -genvall -f $HOME/FlexMPI/controller/rankfiles/rankfile$4 -np $1 /tmp/epiGraph$4 max $HOME/FLexMPI/examples/EpiGraphFlexMPI/ -cfile $HOME/FlexMPI/run/nodefile2.dat -policy-malleability-triggered -lbpolicy-static -ni 20 -ports $2 $3 -controller $controllernode -IOaction $5

mpiexec -genvall -f $HOME/FlexMPI/controller/rankfiles/rankfile$4 -np $1 /tmp/epiGraph$4 max $HOME/FlexMPI/examples/EpiGraphFlexMPI/ -cfile $HOME/FlexMPI/run/nodefile2.dat -policy-malleability-triggered -lbpolicy-static -ni 20 -ports $2 $3 -controller $controllernode -IOaction $5
