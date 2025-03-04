# ZMQ-Uni-Project
This is my answer to the provided task by my University ([TU-Berlin](https://www.tu.berlin/)).

## Task
The provided task was to create a program, which can perform the [MapReduce-Algorithm](https://en.wikipedia.org/wiki/MapReduce).
There had to be a multithreaded [worker](src/worker/main.c) and a multithreaded [distributor](src/distributor/main.c).
The distributor has to queue tasks and handle the communication with the workers (both using [ZMQ](https://zeromq.org/)).

I also implemented a generic [linked list](src/lib/linked_list.h), as well as a generic [hashmap](src/lib/hashmap.h) (utilising the linked list).

You can read more on this [here](praxis3.pdf).

## Build Process
You can run this project by calling the bash scripts I wrote:

### To run the tests provided by the coursework (the testbench is written in python and thus the provided [requirements](requirements.txt) need to be installed):
```sh
./make_tests.sh
```  
  
### Building and passing your own file and ports (the amount of ports can vary and be whatever you want, as long as it isn't reserved, but it has to be the same on both program calls) as an input (you need two terminals open for this):
```sh
./make_build.sh
```  

Terminal 1:
```sh
./build/worker 5555 5556 5557 5558
```
Terminal 2:
```sh
./build/distributor test.txt 5555 5556 5557 5558
```    


This is a copy of the original repository (which is on Gitlab).
