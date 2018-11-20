
# ruby环境安装：
# 1. http://www.ruby-lang.org/en/downloads下下载ruby源码安装
#    wget https://cache.ruby-lang.org/pub/ruby/2.4/ruby-2.4.5.tar.gz
#    tar xzf ruby-2.4.5.tar.gz && cd ruby-2.4.5 && ./configure && make -j 16 && make install
# 2. https://rubygems.org/gems/redis/versions 下载对应版本的redis-xxx.gem文件并安装
# 	 wget https://rubygems.org/downloads/redis-4.0.3.gem
# 	 gem install redis-4.0.3.gem

. ./env.sh

[ ! -d $bin_dir ] && echo "$bin_dir not exist, exit!" && exit 1

[ $# != 1 ] && echo "usage: $0 <deploy | start | stop | clean>" && exit 1

if [ x$1 == x"deploy" ];then

	for port in "${port_array[@]}"; do
		[ -d $work_dir/redis_${port} ] && echo "Error! $work_dir/redis_${port} already exist!" && exit 1
	done

	for port in "${port_array[@]}"; do
		mkdir -p $work_dir/redis-${port}/conf
		mkdir -p $work_dir/redis-${port}/logs
		mkdir -p $work_dir/redis-${port}/bin

		cp -f $work_dir/settings/redis-${port}.conf $work_dir/redis-${port}/conf
		cp -f $bin_dir/bin/redis-server $work_dir/redis-${port}/bin
	done

	cp -f $proj_dir/src/redis-trib.rb $work_dir
	cp -f $bin_dir/bin/redis-cli $work_dir

	hosts=""
	for port in "${port_array[@]}"; do
		cd $work_dir/redis-${port}/bin
		[ ! -f redis-server-${port} ] && ln -sf $work_dir/redis-${port}/bin/redis-server redis-server-${port}

		cd $work_dir/redis-${port}
		./bin/redis-server-${port} ./conf/redis-${port}.conf 1>&2 2>/dev/null &

		[ -n "$hosts" ] && hosts="$hosts 127.0.0.1:${port}"
		[ ! -n "$hosts" ] && hosts="127.0.0.1:${port}"
	done

	cd $work_dir
	./redis-trib.rb create --replicas 1 $hosts

elif [ x$1 = x"start" ];then
	for port in "${port_array[@]}"; do
		cd $work_dir/redis-${port}/bin
		[ ! -f redis-server-${port} ] && ln -sf $work_dir/redis-${port}/bin/redis-server redis-server-${port}

		cd $work_dir/redis-${port}
		./bin/redis-server-${port} ./conf/redis-${port}.conf 1>&2 2>/dev/null &
	done

elif [ x$1 = x"stop" ];then

	ps -ef | grep redis-server | grep -v grep | awk '{print $2}' | xargs -I {} kill -9 {}

elif [ x$1 == x"clean" ];then

	ps -ef | grep redis-server | grep -v grep | awk '{print $2}' | xargs -I {} kill -9 {}

	for port in "${port_array[@]}"; do
		rm -rf $work_dir/redis-${port}
	done

	rm -f $work_dir/redis-trib.rb
	rm -f $work_dir/redis-cli
	rm -f redis-4.0.3.gem

else
	echo "usage: $0 <deploy | start | stop | clean>" && exit 1
fi
