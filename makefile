CXXFLAGS += -std=c++11

CC = g++

OBJS = DiskManager.o FileManager.o FileOperate.o

CFLAGS = 

value: $(OBJS)
	$(CC) $(CFLAGS) -o fileOs $(OBJS)

DiskManager.o: DiskManager.cpp DiskManager.h

FileManager.o: FileManager.cpp FileManager.h  FileOperate.h

FileOperate.o: FileOperate.cpp FileOperate.h DiskManager.h

clean:
	rm $(OBJS) value