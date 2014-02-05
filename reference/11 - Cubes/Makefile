# makefile for macosx

# note: configuration for ODE build
#./configure CFLAGS="-march=core2 -mfpmath=sse -sse3 -O3" CXXFLAGS="-march=core2 -mfpmath=sse -sse3 -O3" --with-trimesh=none --with-drawstuff=none

default: demo

network_dir  := network

#optflags     := -DDEBUG

optflags     := -DNDEBUG -march=core2 -mfpmath=sse -sse3 -O3 -g -ffast-math -fno-math-errno -funsafe-math-optimizations -ffinite-math-only -fno-trapping-math -fsingle-precision-constant

makedepend     := /usr/X11/bin/makedepend
makefile     := Makefile
compiler     := llvm-g++
linker       := ld
flags        := -I. -I$(network_dir) -I$(network_dir)/generated -DSANDBOX -Iode -Wall
libs         := -lode -lm -lnetwork
frameworks   := -framework Carbon -framework OpenGL -framework AGL -framework Cocoa -framework CoreVideo
pch          := PreCompiled.h.gch
test_objs    := $(patsubst tests/%.cpp,tests/%.o,$(wildcard tests/*.cpp))
client_objs  := $(patsubst client/%.cpp,client/%.o,$(wildcard client/*.cpp))
server_objs  := $(patsubst server/%.cpp,server/%.o,$(wildcard server/*.cpp))
shared_objs  := $(patsubst shared/%.cpp,shared/%.o,$(wildcard shared/*.cpp))
source_files := $(wildcard *.cpp) $(wildcard network/*.cpp) $(wildcard client/*.cpp) $(wildcard server/*.cpp) $(wildcard shared/*.cpp) $(wildcard tests/*.cpp)

ndl_headers:
	@make -C $(network_dir) headers

PreCompiled.d : PreCompiled.h ndl_headers
	@$(makedepend) -f- PreCompiled.h $(flags) > PreCompiled.d 2>/dev/null

-include Precompiled.d

$(pch): PreCompiled.h PreCompiled.d $(makefile)
	$(compiler) PreCompiled.h $(flags)

$(network_dir)/output/libnetwork.a:
	make -C $(network_dir) lib

%.o: %.cpp $(pch) $(makefile) $(network_dir)/output/libnetwork.a
	$(compiler) -c $< -o $@ $(flags) $(optflags)

UnitTest: UnitTest.o $(network_dir)/output/libnetwork.a $(test_objs) $(shared_objs) $(server_objs)
	$(compiler) -o $@ $(flags) $(optflags) -L$(network_dir)/output -Ltests/UnitTest++ -lUnitTest++ UnitTest.o $(net_objs) $(ndl_objs) $(test_objs) $(shared_objs) $(server_objs) $(frameworks) $(libs)

Demo: Demo.o $(network_dir)/output/libnetwork.a $(client_objs) $(server_objs) $(shared_objs)
	$(compiler) Demo.o -o $@ $(flags) $(optflags) -L$(network_dir)/output $(ndl_objs) $(client_objs) $(server_objs) $(shared_objs) $(frameworks) $(libs)

demo: Demo
	@mkdir -pv output
	@rm -f output/*.tga
	./Demo

playback: Demo
	./Demo playback

video: Demo
	./Demo playback video

loc: 
	wc -l *.cpp *.h tests/*.h tests/*.cpp client/*.h client/*.cpp server/*.h server/*.cpp shared/*.h shared/*.cpp 

test: #UnitTest
	make -C $(network_dir) test
#	./UnitTest

clean:
	make -C $(network_dir) clean
	rm -rf *.a *.d *.o *.h.gch *.app \
	$(client_objs) $(server_objs) $(shared_objs) $(test_objs) \
	*.bak *.zip *.bin *.dSYM UnitTest Demo output

files_to_zip := $(wildcard *.rb) $(wildcard *.cpp) $(wildcard *.h) $(wildcard tests/*.cpp) $(wildcard tests/*.h) $(wildcard demos/*.h) Makefile

Cubes.zip: $(files_to_zip)
	zip -9 Cubes.zip $(files_to_zip)

zip: Cubes.zip

commit: clean
	git add *
	git commit -a -m "$(m)"

depend: 
	@$(makedepend) -- $(flags) -- $(wildcard *.h *.cpp test/*.h test/*.cpp)

.PHONY: loc
.PHONY: test
.PHONY: demo

# DO NOT DELETE THIS LINE -- make depend depends on it.

PreCompiled.o: /usr/include/stdint.h /usr/include/stdio.h
PreCompiled.o: /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h
PreCompiled.o: /usr/include/sys/_posix_availability.h
PreCompiled.o: /usr/include/Availability.h
PreCompiled.o: /usr/include/AvailabilityInternal.h /usr/include/_types.h
PreCompiled.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
PreCompiled.o: /usr/include/i386/_types.h /usr/include/secure/_stdio.h
PreCompiled.o: /usr/include/secure/_common.h /usr/include/assert.h
PreCompiled.o: /usr/include/unistd.h /usr/include/sys/unistd.h
PreCompiled.o: /usr/include/sys/select.h /usr/include/sys/appleapiopts.h
PreCompiled.o: /usr/include/sys/_structs.h /usr/include/sys/_select.h
PreCompiled.o: network/netConfig.h /usr/include/stdlib.h
PreCompiled.o: /usr/include/sys/wait.h /usr/include/sys/signal.h
PreCompiled.o: /usr/include/machine/signal.h /usr/include/i386/signal.h
PreCompiled.o: /usr/include/i386/_structs.h /usr/include/sys/resource.h
PreCompiled.o: /usr/include/machine/endian.h /usr/include/i386/endian.h
PreCompiled.o: /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h
PreCompiled.o: /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h
PreCompiled.o: /usr/include/machine/types.h /usr/include/i386/types.h
PreCompiled.o: /usr/include/string.h /usr/include/strings.h
PreCompiled.o: /usr/include/secure/_string.h /usr/include/limits.h
PreCompiled.o: /usr/include/machine/limits.h /usr/include/i386/limits.h
PreCompiled.o: /usr/include/i386/_limits.h /usr/include/sys/syslimits.h
PreCompiled.o: /usr/include/math.h /usr/include/architecture/i386/math.h
PreCompiled.o: network/netAssert.h network/netConfig.h
PreCompiled.o: /usr/include/execinfo.h network/netLog.h network/netStream.h
PreCompiled.o: network/netBitPacker.h network/netUtil.h network/netAssert.h
PreCompiled.o: network/netLog.h network/netDataTypes.h network/netStats.h
PreCompiled.o: network/netForwards.h client/Platform.h client/Render.h
PreCompiled.o: client/View.h shared/Mathematics.h shared/Config.h
PreCompiled.o: shared/Simulation.h shared/Activation.h shared/Mathematics.h
PreCompiled.o: shared/Game.h shared/Engine.h shared/Activation.h
PreCompiled.o: shared/Simulation.h shared/ViewObject.h shared/Config.h
Demo.o: PreCompiled.h /usr/include/stdint.h /usr/include/stdio.h
Demo.o: /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h
Demo.o: /usr/include/sys/_posix_availability.h /usr/include/Availability.h
Demo.o: /usr/include/AvailabilityInternal.h /usr/include/_types.h
Demo.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
Demo.o: /usr/include/i386/_types.h /usr/include/secure/_stdio.h
Demo.o: /usr/include/secure/_common.h /usr/include/assert.h
Demo.o: /usr/include/unistd.h /usr/include/sys/unistd.h
Demo.o: /usr/include/sys/select.h /usr/include/sys/appleapiopts.h
Demo.o: /usr/include/sys/_structs.h /usr/include/sys/_select.h
Demo.o: network/netConfig.h /usr/include/stdlib.h /usr/include/sys/wait.h
Demo.o: /usr/include/sys/signal.h /usr/include/machine/signal.h
Demo.o: /usr/include/i386/signal.h /usr/include/i386/_structs.h
Demo.o: /usr/include/sys/resource.h /usr/include/machine/endian.h
Demo.o: /usr/include/i386/endian.h /usr/include/sys/_endian.h
Demo.o: /usr/include/libkern/_OSByteOrder.h
Demo.o: /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h
Demo.o: /usr/include/machine/types.h /usr/include/i386/types.h
Demo.o: /usr/include/string.h /usr/include/strings.h
Demo.o: /usr/include/secure/_string.h /usr/include/limits.h
Demo.o: /usr/include/machine/limits.h /usr/include/i386/limits.h
Demo.o: /usr/include/i386/_limits.h /usr/include/sys/syslimits.h
Demo.o: /usr/include/math.h /usr/include/architecture/i386/math.h
Demo.o: network/netAssert.h network/netConfig.h /usr/include/execinfo.h
Demo.o: network/netLog.h network/netStream.h network/netBitPacker.h
Demo.o: network/netUtil.h network/netAssert.h network/netLog.h
Demo.o: network/netDataTypes.h network/netStats.h network/netForwards.h
Demo.o: client/Platform.h client/Render.h client/View.h shared/Mathematics.h
Demo.o: shared/Config.h shared/Simulation.h shared/Activation.h
Demo.o: shared/Mathematics.h shared/Game.h shared/Engine.h
Demo.o: shared/Activation.h shared/Simulation.h shared/ViewObject.h
Demo.o: shared/Config.h network/netSockets.h demos/SingleplayerDemo.h
Demo.o: demos/Demo.h shared/Hypercube.h demos/DeterministicLockstepDemo.h
UnitTest.o: network/netSockets.h network/netConfig.h
