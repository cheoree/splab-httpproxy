/************************************************************************************************
ASSIGNMENT #3 :  Proxy Cache Server
NAME          :  배철희

DATE          :  2000. 12. 1
COMPILER      :  UNIX gcc 2.8.1,  LINUX gcc 2.9.0
FILE          :  socket.c,  cache.c,  md5.c,  proxy.h
*************************************************************************************************/

/*
 실행시 log파일 생성 구현부는 그대로 두었습니다.
 존재하지 않는 url입력시 core파일이 생성되면서 웹브라우져에 뜨는 에러메시지 페이지를 proxy 
 server에서 임의로 통제해서 보여주는 error.html파일 생성
*/

#include "proxy.h"

static void SIG_CLD(int);   /* fork된 자식 프로세스 종료 포착함수.*/
static void MY_ALARM(int);  /* socket을 통한 통신의 무한대기 방지.*/

const int nSockVal = 1;


int main()
{
    pid_t  pid;
    int    n_cli, cli, n;
    char   buf[BUFFSIZE];
    char   *hash_input;
    struct sockaddr_in proxy_serv;    

    printf("Cheoree's Proxy server started.\n");

    /*******   socket()  ************************************************************************/
    if( (listen_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
	perror("Cannot open stream socket");
	exit(1);
    }
    setsockopt( listen_sd, SOL_SOCKET, SO_REUSEADDR, &nSockVal, sizeof(nSockVal) );
    
    bzero( (char *)&proxy_serv, sizeof(proxy_serv) );

    proxy_serv.sin_family = AF_INET;                 
    proxy_serv.sin_addr.s_addr = htonl(INADDR_ANY);  
    proxy_serv.sin_port = htons(PORTNUM);
    
    /*******   bind()   *************************************************************************/
    if( bind(listen_sd, (struct sockaddr*) &proxy_serv, sizeof(proxy_serv)) < 0 )
    {
	perror("Cannot bind local address\n");
	exit(1);
    }
    
    /*******   listen()  ************************************************************************/
    listen( listen_sd, MAX_NUM_CLIENT );
    
    
    /*---------- Starting loop -----------------------------------------------------------------*/
    for(;;)
    { 
	n_cli = sizeof(cli);
	bzero(buf, sizeof(buf));  
        bzero( (char*) &cli, n_cli );	

        /******  accept()  **********************************************************************/ 
        do{
           accept_sd = accept(listen_sd, (struct sockaddr*) &cli, &n_cli); 
        }while(accept_sd == -1 && errno == (EINTR || EAGAIN) );  

        if( signal(SIGCHLD, SIG_CLD) == SIG_ERR ) 
            perror("signal error.");

	if( (pid = fork()) < 0 )   
            perror("fork error");
        
        /* Child Process Generated */
	else if( pid == 0)
	{
            if( accept_sd == -1 ) 
               exit(1);
 
            close(listen_sd);   

            signal(SIGALRM, MY_ALARM);  
                     
            do{    /*브라우져의(Client) REQUEST를 받는다.*/
               alarm(TIMEOUT);
               n = read(accept_sd, buf, BUFFSIZE);
            }while( n == -1 && errno == EINTR );  
        
            /*위의 REQUEST를 hashing*/
            hash_input = GET_HASH_URL(buf);
  
            /*hashed URL을 입력값으로 작업*/
            /*자식이 수행하는 일의 대부분을 처리하는 함수. (cache.c 파일에 있슴.) */
            CACHEING( hash_input, buf ); 

	    exit(0);
	}
	/* Child exit             */
        close(accept_sd); /*부모 프로세스의 socket descriptor는 닫아줌.*/
    }                  
    /*-------- End of loop  -------------------------------------------------------------------*/
}                    

/***********************************************************************************************
MISS 일때 수행되는 함수로서 서버의 response(header, data)를 읽어와서 웹브라우져로 전송하고 다시 cache
로 저장하는 일을 한다.
************************************************************************************************/
void  CONNECT_WEB_SERV( char *buf, char *whole_dir )
{
   int qfile, write_n, read_n, a, b; 
   char domain_name[DNSIZE], response[BUFFSIZE];
   char ws_request[BUFFSIZE];

   struct hostent *hp; /*웹서버의 주소를 도메인으로 받기위한 주소 구조체.*/
   struct sockaddr_in web_serv;

   bzero(response, sizeof(response));      
   bzero(domain_name, sizeof(domain_name));
   bzero(ws_request, sizeof(ws_request));

   /****** socket()  **************************************************************************/
   if( (connect_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
   {
      perror("Cannot open stream socket.");
      exit(1);
   }
   
   /*domain name만을 parsing*/
   for(a=0;a<DNSIZE;a++)
   {                        
      if( buf[a] == '/' )
      {
         for(b=0;b<DNSIZE-a;b++)
         {
            if( buf[a+b+2] == '/' )
               break;
            domain_name[b] = buf[a+b+2]; 
         }
         break;                                 
      }
   }
  
   /* 웹서버의 주소에 대한 정보를 받는다.*/
   if( (hp = gethostbyname(domain_name)) == NULL )             
   {
      ERRPAGE(domain_name);               /*추가 구현 함수*/       
      ACCESS_LOCAL_CACHE("./error.html"); /*      "      */
      exit(1);  
   }

   bzero( (char*) &web_serv, sizeof(web_serv) );

   web_serv.sin_addr = *( (struct in_addr*)hp->h_addr_list[0] );                               
   web_serv.sin_family = AF_INET;                                                      
   web_serv.sin_port   = htons(WEBPORT); 
            
   /******  connect()  ************************************************************************/                                                                                             
   if( connect(connect_sd, (struct sockaddr*)&web_serv, sizeof(web_serv)) < 0)         
   { 
      perror("Connect error.");
      exit(1);
   } 
           
   /* 웹서버가 Moved등의 에러로 제대로 된 response를 받아올 수 없게 되기때문에 request header
   를 다시 파싱하는 함수이다.*/        
   GET_WS_REQUEST(buf, ws_request);

   /* cache에 웹서버로부터 받아올 데이타를 저장할 파일을 위해 open() */
   qfile = open( whole_dir, O_WRONLY | O_CREAT, 0666 );  
   
   write(connect_sd, ws_request, BUFFSIZE ); /* 웹서버로 request header 전송.*/
 
   do{                                                                   
      alarm(TIMEOUT);                                                   
      while( (read_n = read(connect_sd, response, MAXLINE )) > 0 )       
      {                                                                 
          do{                                                            
             write_n = write(accept_sd, response, read_n);/*클라이언트로 데이타 전송.*/  
             write( qfile, response, read_n );            /*cache로 데이타 저장.    */
             bzero(response, sizeof(response)); 
          }while( write_n == -1 && errno == EINTR);                                                                                                    
      }                                                                 
   }while( read_n == -1 && errno == EINTR );                            

   close(qfile);       
   close(accept_sd);       /* 파일 디스크립터, 소켓디스크립터를 모두 닫는다. 연결 종료.*/              
   close(connect_sd);

   printf("MISS : [ip:%s] ", inet_ntoa(web_serv.sin_addr));
   printf("%s\n", strtok(buf, "\n"));

}

/*******************************************************************************************
HIT 일때 수행되는 함수로서 웹서버로의 connect수행 구현은 제거된다. local server의 cache를 Access
*******************************************************************************************/
void  ACCESS_LOCAL_CACHE( char *whole_dir )
{
   int qfile, read_n; 
   char filebuf[BUFFSIZE];

   qfile = open( whole_dir, O_RDONLY ); 
   while( (read_n = read(qfile, filebuf, MAXLINE )) > 0 )       
   {                                                                 
       write(accept_sd, filebuf, read_n);  
       bzero(filebuf, sizeof(filebuf)); 
   }                                                                 
   printf("HIT  : %s\n", whole_dir);
   close(qfile);                   
}

/**************************************************************************
REQUEST header에서 http://xxx.xxx.xxx/xxx/xxx/xxx.xx 부분만을 잘라내는 함수.
***************************************************************************/
char *  GET_HASH_URL( char *buf ) 
{
   char temp[BUFFSIZE];
   char *hash_input;
  
   bzero(temp, sizeof(temp) );

   strncpy(temp, buf, BUFFSIZE);

   hash_input = strtok(temp, " ");
   hash_input = strtok(NULL, " ");

   return hash_input;
}

/**************************************************
domain name이 제거된 REQUEST header를 얻어내는 함수.
***************************************************/
void  GET_WS_REQUEST( char *buf, char *ws_request)
{
   int i,j;
   int a=0, b=0;

   /* ' '를 만날때까지 복사해나간다. ' '부분의 배열위치를 저장. */
   for(i=0;i<BUFFSIZE;i++)
   {
       ws_request[i] = buf[i]; 
       if( buf[i] == ' ' ) break;
   }
 
   /* '/'문자가 3번 나올때까지 배열을 뛰어넘는다. 3번나왔을때 위치저장.*/
   while( b < 3 )
      if( buf[a++] == '/' ) 
         b++;
   /* ' '문자부터 '/'문자가 3번 나온곳까지를 뛰어넘고 다시 배열 복사.*/
   for(j=0;j<BUFFSIZE-i;j++)
   {
      ws_request[j+i+1] = buf[a+j-1];
      if( buf[j] == '\0')
         break;
   }
} 


static void  MY_ALARM( int signo )
{
   if( signal(SIGALRM, MY_ALARM) ==  SIG_ERR)
      perror("signal error");
   close(accept_sd);
   close(connect_sd);
   printf("Alarm fucntion called.\n");
   exit(0);
}


static void  SIG_CLD( int signo )
{
   pid_t pid;
   int status;
   //printf("child terminated.\n");
   if( signal(SIGCHLD, SIG_CLD) == SIG_ERR )
      perror("signal error");
   if( (pid = wait(&status)) < 0)
      perror("wait error");
   return;
}


/*************************************************************************
추가 구현 함수로서 hostent 주소 구조체의 리턴된 포인터값이 널일 경우 즉, 존재
하지 않는 url이 입력으로 들어왔을때 (proxy server에서) 임의로 웹브라우져에게 
보여질 웹페이지를 만든다.
**************************************************************************/ 
void ERRPAGE(char *url)
{
    char msg[300];
    int er;

    bzero(msg, sizeof(msg));
    printf("domain: %s\n", url);

    sprintf(msg, "<html><title>Connect Error - Cheoree Proxy Server</title>
    <body bgcolor = silver><br><font size = 2><b>Error</b> - http://%s/ <b>is Invalid URL</b>
    </font></body></html>", url);

    er = open("error.html", O_WRONLY | O_CREAT, 0644 );
    write(er, msg, 300);
    close(er);
}

/*************************  End of File  ****************************************************/
