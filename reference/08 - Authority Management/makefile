# makefile for macosx

flags = -msse3 -mfpmath=sse -march=prescott -O3 -Wall -Iode
#flags = -Wall -Iode -DDEBUG

libs = -Lode -lode -lruby
frameworks = -framework Carbon -framework OpenGL -framework AGL

% : %.cpp Display.h Platform.h Mathematics.h NetStream.h
	g++ $< -o $@ ${flags} ${libs} ${frameworks}

%.app : %
	rm -rf $<.app
	mkdir $<.app
	mkdir $<.app/Contents
	mkdir $<.app/Contents/MacOS
	cp $< $<.app/Contents/MacOS/
	
authority : Authority.app Authority
	open Authority.app
	
all : authority

clean:
	rm -rf Authority Authority.app