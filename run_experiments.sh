#!/bin/bash

if [ $# -le 1 ] 
then 
	echo "Usage: ./run_experiments.sh num_timeslots output_dir"
	exit 1
fi 

num_runs=$1
dir=$2

nthreads=1

mkdir $dir

echo "Running all experiments..."

echo "    ...k"
./bin/queuesim -t $nthreads --k_min=2 --k_max=20 --k_step=2 --b_min=30 --b_max=30 --maxval=10 --num_runs=$num_runs --lmb=0.25 > $dir/k.B30.V10.txt
./bin/queuesim -t $nthreads --k_min=2 --k_max=20 --k_step=2 --b_min=100 --b_max=100 --maxval=10 --num_runs=$num_runs --lmb=0.25 > $dir/k.B100.V10.txt
./bin/queuesim -t $nthreads --k_min=2 --k_max=20 --k_step=2 --b_min=300 --b_max=300 --maxval=30 --num_runs=$num_runs --lmb=0.25 > $dir/k.B300.V30.txt

echo "    ...B"
./bin/queuesim -t $nthreads --k_min=50 --k_max=50 --b_min=20 --b_max=60 --b_step=5 --maxval=10 --num_runs=$num_runs --lmb=0.25 > $dir/B.k50.V10.txt
./bin/queuesim -t $nthreads --k_min=10 --k_max=10 --b_min=100 --b_max=5000 --b_step=250 --maxval=25 --num_runs=$num_runs --lmb=0.25 > $dir/B.k10.V25.txt
./bin/queuesim -t $nthreads --k_min=10 --k_max=10 --b_min=100 --b_max=5000 --b_step=250 --maxval=10 --num_runs=$num_runs --lmb=0.25 > $dir/B.k10.V10.txt

echo "    ...V"
./bin/queuesim -t $nthreads --k_min=10 --k_max=10 --b_min=30 --b_max=30 --min_maxval=5 --max_maxval=50 --num_runs=$num_runs --lmb=0.25 > $dir/V.k10.B30.txt
./bin/queuesim -t $nthreads --k_min=10 --k_max=10 --b_min=100 --b_max=100 --min_maxval=5 --max_maxval=50 --num_runs=$num_runs --lmb=0.25 > $dir/V.k10.B100.txt
./bin/queuesim -t $nthreads --k_min=50 --k_max=50 --b_min=300 --b_max=300 --min_maxval=5 --max_maxval=50 --num_runs=$num_runs --lmb=0.25 > $dir/V.k50.B300.txt

echo "    ...lambda"
./bin/queuesim -t $nthreads --k_min=10 --k_max=10 --b_min=100 --b_max=100 --min_maxval=10 --max_maxval=10 --num_runs=$num_runs --total_l=100 > $dir/lmb.k10.B100.V10.txt
./bin/queuesim -t $nthreads --k_min=10 --k_max=10 --b_min=100 --b_max=100 --min_maxval=10 --max_maxval=10 --num_runs=$num_runs --total_l=100 > $dir/lmb.k50.B100.V10.txt
./bin/queuesim -t $nthreads --k_min=10 --k_max=10 --b_min=100 --b_max=100 --min_maxval=30 --max_maxval=30 --num_runs=$num_runs --total_l=100 > $dir/lmb.k10.B100.V30.txt

echo "    ...beta"
for bt in 1 1.2 1.4 1.6 1.8 2 2.2 2.4 2.6
do
	./bin/queuesim -t $nthreads --k_min=10 --k_max=10 --b_min=100 --b_max=100 --min_maxval=10 --max_maxval=10 --num_runs=$num_runs --lmb=0.25 --beta=$bt > $dir/beta$bt.k10.B100.V10.txt
	./bin/queuesim -t $nthreads --k_min=10 --k_max=10 --b_min=100 --b_max=100 --min_maxval=30 --max_maxval=30 --num_runs=$num_runs --lmb=0.25 --beta=$bt > $dir/beta$bt.k10.B100.V30.txt
	./bin/queuesim -t $nthreads --k_min=50 --k_max=50 --b_min=100 --b_max=100 --min_maxval=10 --max_maxval=10 --num_runs=$num_runs --lmb=0.25 --beta=$bt > $dir/beta$bt.k50.B100.V10.txt
done

echo "All done!"
exit 0

