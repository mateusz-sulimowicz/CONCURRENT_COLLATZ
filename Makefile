all:
	export LD_LIBRARY_PATH=/opt/gcc-11.2/lib64
	/opt/gcc-11.2/bin/g++-11.2 -std=c++20 -pthread -o main main.cpp teams.cpp
