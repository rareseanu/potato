CC = gcc

TARGET = debugger
OBJS = debugger.o
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

