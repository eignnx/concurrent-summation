# concurrent-summation
A multi-threaded summation example written in C. The following tests were run on a 2012 MacBook Pro running Ubuntu 16.04 LTS.

## Demo

### Download, then compile with `make`

```shell
$ git clone git@github.com:eignnx/concurrent-summation.git
$ cd concurrent-summation
$ make
```

### Sum the numbers from 1 to 1,000,000,000. Run in single thread.

```shell
$ time ./main 1000000000 --method single

INFO: Spawning 1 thread...

INFO: Thread exited with status 0.

Expected:   500000000500000000
Calculated: 500000000500000000

real	0m2.573s
user	0m2.572s
sys	0m0.001s

```

### Spawn as many threads as there are available CPU cores.

```shell
$ time ./main 1000000000 --method cpus

INFO: Spawning 4 threads...

INFO: Thread 0 gets [1, 250000001).
INFO: Thread 1 gets [250000001, 500000001).
INFO: Thread 2 gets [500000001, 750000001).
INFO: Thread 3 gets [750000001, 1000000001).

INFO: Thread 0 exited with status 0.
INFO: Thread 1 exited with status 0.
INFO: Thread 2 exited with status 0.
INFO: Thread 3 exited with status 0.

Expected:   500000000500000000
Calculated: 500000000500000000

real	0m0.988s
user	0m3.902s
sys	0m0.004s
```
