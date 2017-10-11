# Register Sharing

This repository contains the implementation of GPU register sharing,  which is discussed in our HPDC 2016 paper entitled "Improving GPU Performance Through Resource Sharing". The project is built on the top of GPGPU-Sim v3.x simulator. 

## Authors:

* Vishwesh Jatala ([vjatala@cse.iitk.ac.in](mailto:vjatala@cse.iitk.ac.in)), Indian Institute of Technology Kanpur
* Jayvant Anantpur ([jayvant.anantpur@gmail.com](mailto:jayvant.anantpur@gmail.com)), Mentor Graphics India Pvt Ltd
* Amey Karkare( [karkare@cse.iitk.ac.in](mailto:karkare@cse.iitk.ac.in)), Indian Institute of Technology Kanpur

## Prerequisites

Since this repository is built on the top of GPGPU-Sim simulator, the software requirement of our implementation is same as that of GPGPU-Sim v3.x simulator, and no other software is additionally required.

Note that all the software requirements for the GPGPU-Sim simulator are listed in the directory Baseline\_GPGPU-Sim/v3.x/README file.


## Directory Structure

The top-level directory contains following subdirectories

* Baseline\_GPGPU-Sim  (This is the baseline GPGPU-Sim v3.x simulator) 
* RegisterSharing\_GPGPU-Sim (This contains the simulator that supports Register Sharing)
*  Config (Configuration files required for Baseline\_GPGPU-Sim  and RegisterSharing\_GPGPU-Sim simulators)
* Benchmarks (Benchmark applications. Note that we only provided the source code of the benchmarks and not datasets because  of their huge size.  Please download them from the respective benchmark suites.)
* PTXPLUS_FILES (Contains the optimized ptxplus files that have unrolled and reordered register declarations, see our HPDC 2016 for details).
* README.md (The readme file)

## Installation

### Installation of Simulators (Baseline\_GPGPU-Sim and RegisterSharing\_GPGPU-Sim)
The steps for installing RegisterSharing_GPGPU-Sim is same as that of Baseline\_GPGPU-Sim.

Assuming that all the dependencies for RegisterSharing\_GPGPU-Sim are resolved, and assuming that your cuda-toolkit is installed in /usr/local/cuda, then to install the RegisterSharing\_GPGPU-Sim add the following line to ~/.bashrc file. 

```
$ export CUDA_INSTALL_PATH=/usr/local/cuda
```

Execute the following commands in sequence to complete the installation of RegisterSharing\_GPGPU-Sim

```
$ cd  RegisterSharing_GPGPU-Sim
```

```
$ bash 
```

```
$ source setup_environment
```

```
$ make
```

### Installation of Benchmark Applications

The benchmark applications that we use for the experiments are from Rodinia, ispass2009-benchmarks, Parboil. The steps for installing the benchmarks are given in readme files located in the corresponding benchmark suites.  


## Executing Benchmark Applications on Simulators

Executing the benchmark applications on the simulator requires configuring the simulator. The configuration file to configure the RegistersSharing implementation is listed under "Config/RegisterSharing" directory. Before running the experiments, the configuration files must be copied the application working directory. For example, to configure the RegisterSharing for "hotspot" application (Rodnina benchmark), perform the following steps:

Step-1:  Copy the configuration files of RegisterSharing into the application directory.

```
cp Config/RegisterSharing/* Benchmarks/rodinia_2.4/cuda/hotspot/
```

Step-2: Configure hotspot for RegisterSharing_GPGPU-Sim simulator.

```
source  RegisterSharing_GPGPU-Sim/v3.x/setup_environment
```

Step-3: Copy the PTXPLUS files.  As discussed in our paper, the ptxplus file needs to be optimized before they can be simulated on our implementation. Gernating the PTXPLUS is manual, however, we provided all the optimized ptxplus files for applicable benchmarks in the PTXPLUS_FILES directory (for cuda-4.0 and cuda-4.2 versions) for convenience.  For generating the optimized ptxplus, the process is explained in the paper and is simple.  In future, we aim to automated this process by integrating with the compiler phase itself.

```
cp  PTXPLUS_FILES/Set-1/cuda-4.0/hotspot/ptxplus_UNROLLED Benchmarks/rodinia_2.4/cuda/hotspot/
```

Step-4: Enter the hotspot direcoty

```
cd Benchmarks/rodinia_2.4/cuda/hotspot/
```

Step-5: Execute hotspot application

```
./run
```

This executes the hotspot application with the RegisterSharing simulator and prints the statistics on the console. To compare our implementation with the Baseline\_GPGPU-Sim, the hotspot needs to be configured  and executed for Baseline\_GPGPU-SIm simulator, which is same as the above steps (except that it does not requie Step-3).

Similarly, any other benchmark application can be executed using the above steps.

## Acknowledgments
Vishwesh Jatala is supported by TCS Ph.D. fellowship for his Ph.D. program.

