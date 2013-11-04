CPP=g++
CC=gcc

GS_PROCESS_SOURCES=gsmstack.c cch.c sch.c interleave.c out_pcap.c fire_crc.c
GS_PROCESS_OBJECTS=$(GS_PROCESS_SOURCES:.c=.o)
BUILDDIR=../build

all: $(GS_PROCESS_SOURCES) $(GS_PROCESS_OBJECTS)

.c.o:
	@mkdir -p $(BUILDDIR)
	$(CC) -c $< -o $(BUILDDIR)/$@


clean:
	rm -rf ../build


