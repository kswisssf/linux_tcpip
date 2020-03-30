#! /bin/sh 

# Shell赋值时,等号左右两边是不能有空格的
bin1=tcp_server
bin2=tcp_server2
bin3=IO_multi_cycle_server

function make()
{	
	gcc  $bin1.c -o $bin1 
	gcc  $bin2.c -o $bin2 -lpthread
	gcc  $bin3.c -o $bin3 -lpthread	
}

function clean()
{
	rm -rf $bin1 
	rm -rf $bin2
	rm -rf $bin3	
	rm -rf *.o *.d	
}

if [ x"$1" = x ]; then 
    echo "no cmd param!"
    exit 1
fi
#看到提示错误：“syntax error near unexpected token `then'”
#问题在于空格，这个很难发现，if和“[”之间要有空格，“==”两边也要有空格。
if [ "$1" = "make" ]; then
	make
	echo "make finish!"
	exit 0
fi

if [ "$1" = "clean" ]; then
	clean
	echo "clean finish!"
	exit 0
fi