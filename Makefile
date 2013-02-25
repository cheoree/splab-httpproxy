
OBJECTS  = md5.o cache.o socket.o
SOURCE   = md5.c cache.c socket.c
SOCK_OPT = -lsocket -lnsl
WALL     = -Wall 
CC       = gcc


pserv : $(OBJECTS) 
	$(CC) -o pserv $(SOURCE) $(SOCK_OPT) $(WALL) 
	@echo "\n* 성공적으로 컴파일되었습니다. *\n* 실행시 ./pserv를 입력하세요. *\n ";

md5.o : md5.c proxy.h 
	$(CC) -c md5.c $(WALL) 

cache.o : cache.c proxy.h
	$(CC) -c cache.c $(WALL)  

socket.o : socket.c proxy.h
	$(CC) -c socket.c $(WALL)  

clean :
	@rm -rf $(OBJECTS) pserv cache log    
