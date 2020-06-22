ifeq ($(shell uname), Linux)
   LINUX_BT = 1 # remove or comment this line if compiling in Linux without BlueZ development files
endif

ifdef LINUX_BT
   CFLAGS = -DHASBLUETOOTH
   LIBS = -lbluetooth
endif 

all: acquisition

acquisition: bitalino.o acquisition.o
	g++ $^ $(LIBS) -o $@

bitalino.o: bitalino.cpp
	g++ $(CFLAGS) -c $< -o $@

acquisition.o: acquisition.cpp
	g++ -std=c++0x -c $< -o $@