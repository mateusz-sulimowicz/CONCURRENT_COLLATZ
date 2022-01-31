all:
	g++-11 -std=c++2a -pthread -o main main.cpp teams.cpp
	g++-11 -std=c++2a -pthread -o new_process new_process.cpp
