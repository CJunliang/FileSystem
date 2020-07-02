$(CC) = gcc

OBJS = DiskManager.o FileManager.o FileOperate.o

CFLAGS = 

all:$(OBJS)
	$(CC) -o fileOs $(OBJS)

DiskManager.o: DiskManager.cpp DiskManager.h
	$(CC) $(CFLAGS) -c DiskManager.cpp

FileManager.o: FileManager.cpp FileManager.h 
	$(CC) $(CFLAGS) -c FileManager.cpp

FileOperate.o: FileOperate.cpp FileOperate.h DiskManager.h
	$(CC) $(CFLAGS) -c FileOperate.cpp

clean:
	-rm DiskManager.o
	-rm FileManager.o
	-rm FileOperate.o