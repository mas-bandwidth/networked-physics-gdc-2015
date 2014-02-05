# makefile for macosx

flags = -Wall -DDEBUG # -O3

% : %.cpp NetStream.h
	g++ $< -o $@ ${flags}

all : Example Test

example : Example
	./Example
	
test : Test
	./Test
	
clean:
	rm -f Client Server Test
