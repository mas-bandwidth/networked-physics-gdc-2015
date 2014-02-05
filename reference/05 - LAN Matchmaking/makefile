# makefile for macosx

flags = -Wall -DDEBUG # -O3

% : %.cpp Net.h
	g++ $< -o $@ ${flags}

all : Beacon

beacon : Beacon
	./Beacon
	
clean:
	rm -f Beacon
