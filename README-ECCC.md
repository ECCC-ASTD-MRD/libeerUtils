# libeerUtils Description
libeerUtils contains a suite of base utility functions used by SPI and EER's dispersion models and tools

# Getting the source code
```shell
git clone --recursive git@gitlab.science.gc.ca:ECCC_CMOE_APPS/libeerutils
```
# Building libeerUtils
You will need cmake with a version at least 3.12
```shell
. ssmuse-sh -x /fs/ssm/main/opt/cmake-3.21.1
```

## Depdencies
* Mandatory external dependencies are available through the [SPI-Externals](https://gitlab.science.gc.ca/ECCC_CMOE_APPS/SPI-External). This can extracted in ans SSM_DEV directory, built and provided to the build through the TCL_SRC_DIR variable. See below for Build process

## Optional dependencies
* codetools and compilers
```shell
. r.load.dot rpn/code-tools/ENV/cdt-1.5.3-intel-19.0.3.199
```

* [librmn](https://gitlab.science.gc.ca/RPN-SI/librmn)
```shell
. r.load.dot rpn/libs/19.7.0
```

* [vgrid](https://gitlab.science.gc.ca/RPN-SI/vgrid)
```shell
. r.load.dot rpn/vgrid/6.5.0
```

* External dependencies ([GDAL](https://gdal.org/), [eccodes](https://confluence.ecmwf.int/display/ECC), [libecbufr](https://github.com/ECCC-MSC/libecbufr), [fltlib](https://sourceforge.net/projects/fltlib)). Within the ECCC/SCIENCE network, a package containing all the dependencies can be loaded
```shell
export CMD_EXT_PATH=/fs/ssm/eccc/cmd/cmds/ext/20210211; . ssmuse-sh -x $CMD_EXT_PATH
```

## Environment setup (At CMC)

Source the right file depending on the architecture you need from the env directory. This will load the specified compiler and define the ECCI_DATA_DIR variable for the test datasets

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

## Build, install and package
```shell
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$SSM_DEV/workspace -DTCL_SRC_DIR=${SSM_DEV}/SPI-External/tcl ../
make -j 4
make test
make install
make package
```
