CC = g++
CFLAGS = -std=c++11 -Wall

SRCS = main.cpp

OBJS = $(SRCS:.cpp=.o)

TARGET = newtplinker_punit

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
