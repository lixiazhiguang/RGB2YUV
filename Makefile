CC=g++
SOURCE=./rgb2yuv.cpp
TARGET=rgb2yuv

all: TARGET

TARGET:
	$(CC) -o $(TARGET) $(SOURCE) -std=c++11 `pkg-config opencv --cflags --libs`

clean:
	rm rgb2yuv
