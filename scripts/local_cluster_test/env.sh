
script_dir=`cd $(dirname $0); pwd`
work_dir=$script_dir
proj_dir=`cd $script_dir/../..; pwd`

bin_dir_name=redis_bin
bin_dir=$proj_dir/$bin_dir_name

port_array=(7000 7001 7002 7003 7004 7005)

# for test two-dimensional array
port_array2=( '5191 5192'
			  '5193 5194' )
for((i=1; i<${#port_array2[@]}; i++)) 
do
	port_array2_1=(${port_array2[$i]})
	for port in "${port_array2_1[@]}"; do
		echo `expr $port + 1` > /dev/null
	done
done
# end test