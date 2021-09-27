# libeerUtils Description

libeerUtils contains a suite of base utility functions used by SPI and EER dispersion models and tools

# Building libeerUtils

## Build dependencies

- CMake 3.12+

### Optional dependencies (On ECCC/SCIENCE network)
[codetools](https://gitlab.science.gc.ca/RPN-SI/code-tools)
```shell
. r.load.dot rpn/codetools/1.5.1
```

[librmn](https://gitlab.science.gc.ca/RPN-SI/librmn)
```shell
. r.load.dot rpn/libs/19.7.0
```

[vgrid](https://gitlab.science.gc.ca/RPN-SI/vgrid)
```shell
. r.load.dot rpn/vgrid/6.5.0
```

External dependencies (GDAL,URP,ECCODES,LIBECBUFR,...). Within the SCIENCE network, a package containing all the dependencies cna be loaded
```shell
export CMD_EXT_PATH=/fs/ssm/eccc/cmd/cmds/ext/20210211; . ssmuse-sh -x $CMD_EXT_PATH
```

### Environment setup (At CMC)

#### Source the right file depending on the architecture you need from the env directory.
This will load the specified compiler and define the ECCI_DATA_DIR variable for the test datasets

- Example for PPP3 and skylake specific architecture:
```shell
. ci-env/latest/ubuntu-18.04-skylake-64/intel-19.0.3.199.sh
```

- Example for XC50 on intel-19.0.5
```shell
. ci-env/latest/sles-15-skylake-64/intel-19.0.5.281.sh
```

- Example for CMC network and gnu 7.5:
```shell
. ci-env/latest/ubuntu-18.04-amd-64/gnu-7.5.0.sh
```

### Build, install and package
```shell
mkdir build
cd build
cmake ../
make -j 4
make test
make install
make package
```
