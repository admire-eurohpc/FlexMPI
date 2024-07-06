list=`cat $1 | uniq`

rm -f nodefile1.dat nodefile2.dat   
for node in $list
do
   val=`grep $node $1 | wc -l`
   echo "$node":$val:"$node" >> nodefile1.dat
   echo "$node":$val:"$node":100:5000 >> nodefile2.dat   
done
