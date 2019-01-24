CC = gcc

XX = g++

CFLASS = -Wall -O -g

TARGET = ./rundemo

%.o:%.c
	$(CC) $(CFLASS) -c $< -o $@
	
	
SOURCES = $(wildcard *.c *.cpp)
OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))

$(TARGET) : $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)
	chmod a+x $(TARGET)
	rm -rf *.o
	
	
clean:
	rm -rf rundemo *.o
