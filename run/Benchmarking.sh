
echo "Executing bandwidth benchmark..."
dir1=`pwd`
node1=`head -1 $1 `
node2=`tail -1 $1 `
echo $node1 > ../utils/rankfile
echo $node2 >> ../utils/rankfile
cd ../utils

mpiexec -np 2 -f ./rankfile ./BandwidthBench > BandwidthBench.out

cd $dir1


