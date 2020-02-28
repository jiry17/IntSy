# IntSy

Artifact for PLDI2020: Question Selection for Interactive Program Synthesis

### Requirement

1. cmake and glog, which can be installed in Ubuntu by the following command.

````bash
$ apt-get install cmake libgoogle-glog-dev python3-pip
````

2. python packages

```bash
$ pip3 install pyparsing z3-solver xlrd matplotlib 
```

### Installation

####Build from source (Test on Ubuntu 16.04) 

1. Clone IntSy and all its submodules (or just unzip `IntSy.zip` shared on Dropbox)

```bash
$ git clone --recursive https://gitee.com/jiry_2/IntSy.git
```

2. Build the whole project. Under the root directory of the project:

```bash
$ ./install
```

#### Docker

We also release a docker container in which this project is already built at `~/IntSy`.

```
docker pull takanashirikka/intsy
```

However, since docker will affect the execution speed, we still recommend building the project from source.

#### Some Tests

1. Test whether *Euphony* is successfully installed:

   ```bash
   $ cd recommend/euphony
   $ . bin/setenv  
   $ ./bin/run_string benchmarks/string/train/firstname.sl
   ```

   The expected output is a program which splits an input string by a space and returns the first substring:

   ```
   (define-fun f ((name String)) String (str.substr name 0 (str.indexof name " " 0)))
   ```

2. Test whther *Eusolver* is successfully installed:

   ```bash
   $ cd recommend/eusolver
   $ ./eusolver benchmarks/max/max_2.sl
   ```

   The expected output is a program which takes two integers as the input and returns the larger one:

   ```
   (define-fun max2 ((a0 Int) (a1 Int)) Int
        (ite (<= a1 a0) a0 a1))
   ```

3. Test whether the project is successfully built:

   ```bash
   $ cd run
   $ ./run_benchmark -t repair -b t1.sl
   ```

   The expected output contains a series of log information describing the interaction between the synthesizer and a simulated developer.  The last line of the log should be a program semantically equivalent with the following one:

   ```
   {== #P10 #P7}
   ```

### Run synthesizers

We provide two scripts `run_benchmark` and `run_run` under the directory `run`: the former one helps run synthesizers one a single benchmark, and the latter one helps reproduce the experiment results.

#### Run synthesizers on a single SyGuS file 

```bash
$ cd run
$ ./run_benchmark -b BENCHMARK [-d DEPTH] [-l LOGFILE] [-s {SampleSy,EpsSy,RandomSy}] -t {string,repair}
```

1. `-b` : the name of the benchmark.
2. `-t` : the type of the benchmark.
3. `-l` : the name of the log file, e.g., if its value is `test.log`,  all the logs will be stored in `log/test.log`. The default value of this flag is `NONE`, which means directly output the logs to STDOUT.
4. `-d ` : the depth of the program space. The default value is stored in `benchmark.xls`.
5. `-s` : the name of the synthesizer. The default synthesizer is *SampleSy*, and the threshold for *EpsSy* is 5.

Some examples are listed below:

```bash
# Run RandomSy on benchmark t1.sl inside the repair track
$ ./run_benchmark -s RandomSy -t repair -b t1.sl
# Run SampleSy on benchmark phone.sl inside the string track with the depth 5
$ ./run_benchmark -t string -b phone.sl -d 5
```

If you want to execute the synthesizers on a new benchmark, the benchmark should be firstly put to the corresponding directories: `benchmarks/repair` for the repair track and `benchmarks/string/final` for the string track.

#### Run experiments 

```bash
$ cd run
$ ./run_exp [-tn THREADNUM] [-m MEMORY <GB>] [-exp {1,2,3,4}] [-r REPEATNUM] [-c {R <Restart>,C <Clear>}]
# For example, to reproduce all results:
$ ./run_exp -c R
```

1. `-tn`: the number of threads. The default value is 8.

2. `-m`: the memory limit for each thread. The default value is 6 GB. Please ensure the number of threads times the memory limit is smaller than the memory size of your computer.

3. `-exp`: the id of the experiment you want to run. All experiments will be executed by default.

4. `-r`: the number of repetitions of each execution. The default value is 5. Note that all the algorithms are random, the smaller this value is, the more volatile the result will be.

5. `-c`: whether to clear the cache: `R` represents yes while `C` represents no, and the default value is `C`. 

The result of each single execution is cached in `run/result.pkl` . `run_exp` will firstly check whether  `result.pkl` contains enough data to draw the figure: only the insufficient part will be executed. 

The figure of each experiment will be stored in `run/figure`.

###Reproduce results in the paper 

#### Results of experiment 1

```bash
$ cd run
$ ./run_exp -exp 1
```

For Figure 2 (Line 1046-1062), the script will redraw it as `run/figure/exp1.png` while the original figure used in our paper can be found as `run/expected/exp1.png`.

For data listed in Section 7.3 (Line 1036-1044, 1065-1068), `run_exp` will recalculate it and print it to the standard output.

#### Results of experiment 2

````bash
$ cd run
$ ./run_exp -exp 2
````

For Figure 3 (Line 1101-1115), the script will redraw it as `run/figure/exp2.png` while the original figure used in our paper can be found in `run/expected/exp2.png`.

#### Results of experiment 3

````bash
$ cd run
$ ./run_exp -exp 3
````

For Figure 4 (Line 1125-1140), the script will redraw it as `run/figure/exp3.png` while the original figure used in our paper can be found as `run/expected/exp3.png`.

For data listed in Section 7.5 (Line 1148-1154), `run_exp` will recalculate it and print it to the standard output.

#### Results of experiment 4

```bash
$ cd run
$ ./run_exp -exp 4
```

For Figure 5 (Line 1167-1179), the script will redraw it as `run/figure/exp4.png` while the original figure used in our paper can be found as `run/expected/exp4.png`.

**Note:** There may be some small differences between the results listed in our paper and the reproduced ones since all the synthesizers in our paper are random. The results of the string track are more stable than those of the repair track since the former one contains much more benchmarks.