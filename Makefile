CC = gcc -lelf -ldwarf -I /usr/include/libdwarf

TARGET = demo
OBJS = demo.o breakpoint.o debugger.o dwarf_utils.o
REBUILDABLES = $(OBJS) $(TARGET) 

clean:
	rm -f $(REBUILDABLES)
	@echo Done cleaning.

all: $(TARGET)
	@echo Done building.

$(TARGET): $(OBJS)
	$(CC) -g -o $@ $^ 

%.0: %.c
	$(CC) -Wall -g -o $@ $<

demo.o : debugger.h breakpoint.h
debugger.o : debugger.h breakpoint.h
breakpoint.o : breakpoint.h dwarf_utils.h
dwarf_utils.o : dwarf_utils.h
