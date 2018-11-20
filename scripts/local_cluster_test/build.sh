
#!/bin/bash

. ./env.sh

if [ -d $bin_dir ];then
	echo "$bin_dir already exist, exit!"
	exit 1
fi

cd $proj_dir

make PREFIX=`pwd`/redis_bin LDFLAGS="-lrt" -j 16
make PREFIX=`pwd`/redis_bin install

cd $script_dir