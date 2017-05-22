# sim-twocharacteristics
Simulation code for "Priority Queueing for Packets with Two Characteristics"

In order to build the basic program `queuesim`, you have to have boost libraries installed; e.g., on an Ubuntu system run
```
sudo apt-get install libboost-dev libboost-random-dev libboost-system-dev libboost-program-options-dev libboost-filesystem-dev libboost-date-time-dev
```

After that, compile the basic program by running
```
make
```

Then, to produce logs with output corresponding to the experiments in the paper "Priority Queueing for Packets with Two Characteristics", run the `run_experiments.sh` script, e.g.,
```
./run_experiments.sh 100000 tmp_output
```
where `100000` is the number of timeslots in each simulation experiment and tmp_output is the directory where the logs will be stored.


