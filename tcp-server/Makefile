CPP=g++
#CC=gcc

CFLAGS=-lpthread -lboost_program_options

MAIN_SOURCES=decoders.cc tcp-server-thread.cc burst_fifo.cpp dealia.c burstdata.cpp burst_process.cc main.c
TMP_OBJECTS=$(MAIN_SOURCES:.cpp=.o) # $(MAIN_SOURCES:.cc=.o)
TMP_OBJECTS2=$(TMP_OBJECTS:.c=.o)
MAIN_OBJECTS=$(TMP_OBJECTS2:.cc=.o)
BUILDDIR=./build
BUILD_OBJECTS=$(wildcard $(BUILDDIR)/*.o)
BINARY=ts


main: $(MAIN_SOURCES) $(MAIN_OBJECTS) ts

	g++ $(CFLAGS) $(BUILD_OBJECTS) -o ./$(BINARY)

ts: 
	cd gs_process; make

.cc.o:
	@mkdir -p $(BUILDDIR)
	$(CPP) -c $< -o $(BUILDDIR)/$@

.c.o:
	@mkdir -p $(BUILDDIR)
	$(CPP) -c $< -o $(BUILDDIR)/$@

.cpp.o:
	@mkdir -p $(BUILDDIR)
	$(CPP) -c $< -o $(BUILDDIR)/$@




####

.SILENT : clean
clean : 
	rm -rf *o ts *.xml *.txt ./build ./$(BINARY)
	
