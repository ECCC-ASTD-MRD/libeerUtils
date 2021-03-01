# libeerUtils Description

libeerUtils contains a suite of base utility functions used by SPI and EER dispersion models and tools

# Building libeerUtils

## Dependencies
You will need cmake with a version at least 3.16
```shell
. ssmuse-sh -x /fs/ssm/main/opt/cmake-3.16.4
```

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

### Environment setup
The build process requires the definition of a variable indicating where the build will occur
```shell
export SSM_DEV=[where to build]
mkdir -p $SSM_DEV/src $SSM_DEV/package $SSM_DEV/workspace $SSM_DEV/build
```

### Launching the build
```shell
makeit -reconf -build -ssm
```
