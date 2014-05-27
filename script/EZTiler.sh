#!/bin/sh

model=$1
run=$2

date=`date +%Y%m%d`
dbase_ops='/data/gridpt/dbase/prog'
dbase_til=''

for f in `ls -1 $dbase_ops/$model/${date}${run}_???`
do
echo "EZTiler ${dbase_ops}/${model}/$f ${dbase_til}/${model}/$f 128 128 UU VV WE GZ P0"
done