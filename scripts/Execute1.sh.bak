#!/bin/bash

if [ "$#" -ne 4 ]; then
    echo "Illegal number of parameters"
    exit 1
fi

NODENAME=`uname -n`

cp  $HOME/FlexMPI/configuration_files/nodefiles/nodefile.demo1 $HOME/FlexMPI/configuration_files/nodefiles/nodefile
sed -i -e 's/nodename/'"$NODENAME"'/g' $HOME/FlexMPI/configuration_files/nodefiles/nodefile

cp $HOME/FlexMPI/configuration_files/corefiles/corefile.demo1 $HOME/FlexMPI/configuration_files/corefiles/corefile
sed -i -e 's/nodename/'"$NODENAME"'/g' $HOME/FlexMPI/configuration_files/corefiles/corefile

cp $HOME/FlexMPI/controller/rankfiles/rankfile1.demo1  $HOME/FlexMPI/controller/rankfiles/rankfile
sed -i -e 's/nodename/'"$NODENAME"'/g' $HOME/FlexMPI/controller/rankfiles/rankfile


list=`cat $HOME/FlexMPI/configuration_files/nodefiles/nodefile |  awk -F: '{print $1}'`

for item in $list; do
  cp $HOME/FlexMPI/examples/jacobi_IO /tmp/jacobi_IO$4
done


$HOME/LIBS/mpich/bin/mpiexec -genvall -f $HOME/FlexMPI/controller/rankfiles/rankfile -np $1 /tmp/jacobi_IO$4 500 10000 0.00001 1 1 0 $4 -cfile $HOME/FlexMPI/configuration_files/corefiles/corefile -policy-malleability-triggered -lbpolicy-counts 15000 100 -ni 100 -ports $2 $3 -controller $NODENAME -IOaction 2


