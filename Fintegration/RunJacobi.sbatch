#!/bin/bash
#SBATCH -e slurm-%j.err
#SBATCH -o slurm-%j.out
#SBATCH --job-name=Jacobi_F
#SBATCH --ntasks=1
#SBATCH --nodelist=slurm-node-1

DIR=/home/admin/FlexMPI/Fintegration/

${DIR}/RunJacobi.sh 2

