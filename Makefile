
OBJECTS  = md5.o cache.o socket.o
SOURCE   = md5.c cache.c socket.c
SOCK_OPT = -lsocket -lnsl
WALL     = -Wall 
CC       = gcc


pserv : $(OBJECTS) 
	$(CC) -o pserv $(SOURCE) $(SOCK_OPT) $(WALL) 
	@echo "\n* ���������� �����ϵǾ����ϴ�. *\n* ����� ./pserv�� �Է��ϼ���. *\n ";

md5.o : md5.c proxy.h 
	$(CC) -c md5.c $(WALL) 

cache.o : cache.c proxy.h
	$(CC) -c cache.c $(WALL)  

socket.o : socket.c proxy.h
	$(CC) -c socket.c $(WALL)  

clean :
	@rm -rf $(OBJECTS) pserv cache log    
