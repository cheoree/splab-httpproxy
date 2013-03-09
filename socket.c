/************************************************************************************************
ASSIGNMENT #3 :  Proxy Cache Server
NAME          :  ��ö��

DATE          :  2000. 12. 1
COMPILER      :  UNIX gcc 2.8.1,  LINUX gcc 2.9.0
FILE          :  socket.c,  cache.c,  md5.c,  proxy.h
*************************************************************************************************/

/*
 ������ log���� ���� �����δ� �״��� �ξ����ϴ�.
 �������� �ʴ� url�Է½� core������ �����Ǹ鼭 ������������ �ߴ� �����޽��� �������� proxy 
 server���� ���Ƿ� �����ؼ� �����ִ� error.html���� ����
*/

#include "proxy.h"

static void sig_cld(int);   /* fork�� �ڽ� ���μ��� ���� �����Լ�.*/
static void my_alarm(int);  /* socket�� ���� ������ ���Ѵ��� ����.*/

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

        if( signal(SIGCHLD, sig_cld) == SIG_ERR ) 
            perror("signal error.");

	if( (pid = fork()) < 0 )   
            perror("fork error");
        
        /* Child Process Generated */
	else if( pid == 0)
	{
            if( accept_sd == -1 ) 
               exit(1);
 
            close(listen_sd);   

            signal(SIGALRM, my_alarm);  
                     
            do{    /*����������(Client) REQUEST�� �޴´�.*/
               alarm(TIMEOUT);
               n = read(accept_sd, buf, BUFFSIZE);
            }while( n == -1 && errno == EINTR );  
        
            /*���� REQUEST�� hashing*/
            hash_input = get_hash_url(buf);
  
            /*hashed URL�� �Է°����� �۾�*/
            /*�ڽ��� �����ϴ� ���� ���κ��� ó���ϴ� �Լ�. (cache.c ���Ͽ� �ֽ�.) */
            cacheing( hash_input, buf ); 

	    exit(0);
	}
	/* Child exit             */
        close(accept_sd); /*�θ� ���μ����� socket descriptor�� �ݾ���.*/
    }                  
    /*-------- End of loop  -------------------------------------------------------------------*/
}                    

/***********************************************************************************************
MISS �϶� �����Ǵ� �Լ��μ� ������ response(header, data)�� �о��ͼ� ������������ �����ϰ� �ٽ� cache
�� �����ϴ� ���� �Ѵ�.
************************************************************************************************/
void  connect_web_srv( char *buf, char *whole_dir )
{
   int qfile, write_n, read_n, a, b; 
   char domain_name[DNSIZE], response[BUFFSIZE];
   char ws_request[BUFFSIZE];

   struct hostent *hp; /*�������� �ּҸ� ���������� �ޱ����� �ּ� ����ü.*/
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
   
   /*domain name���� parsing*/
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
  
   /* �������� �ּҿ� ���� ������ �޴´�.*/
   if( (hp = gethostbyname(domain_name)) == NULL )             
   {
      errpage(domain_name);               /*�߰� ���� �Լ�*/       
      access_local_cache("./error.html"); /*      "      */
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
           
   /* �������� Moved���� ������ ������ �� response�� �޾ƿ� �� ���� �Ǳ⶧���� request header
   �� �ٽ� �Ľ��ϴ� �Լ��̴�.*/        
   get_ws_request(buf, ws_request);

   /* cache�� �������κ��� �޾ƿ� ����Ÿ�� ������ ������ ���� open() */
   qfile = open( whole_dir, O_WRONLY | O_CREAT, 0666 );  
   
   write(connect_sd, ws_request, BUFFSIZE ); /* �������� request header ����.*/
 
   do{                                                                   
      alarm(TIMEOUT);                                                   
      while( (read_n = read(connect_sd, response, MAXLINE )) > 0 )       
      {                                                                 
          do{                                                            
             write_n = write(accept_sd, response, read_n);/*Ŭ���̾�Ʈ�� ����Ÿ ����.*/  
             write( qfile, response, read_n );            /*cache�� ����Ÿ ����.    */
             bzero(response, sizeof(response)); 
          }while( write_n == -1 && errno == EINTR);                                                                                                    
      }                                                                 
   }while( read_n == -1 && errno == EINTR );                            

   close(qfile);       
   close(accept_sd);       /* ���� ����ũ����, ���ϵ���ũ���͸� ���� �ݴ´�. ���� ����.*/              
   close(connect_sd);

   printf("MISS : [ip:%s] ", inet_ntoa(web_serv.sin_addr));
   printf("%s\n", strtok(buf, "\n"));

}

/*******************************************************************************************
HIT �϶� �����Ǵ� �Լ��μ� ���������� connect���� ������ ���ŵȴ�. local server�� cache�� Access
*******************************************************************************************/
void  access_local_cache( char *whole_dir )
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
REQUEST header���� http://xxx.xxx.xxx/xxx/xxx/xxx.xx �κи��� �߶󳻴� �Լ�.
***************************************************************************/
char *  get_hash_url( char *buf ) 
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
domain name�� ���ŵ� REQUEST header�� ����� �Լ�.
***************************************************/
void  get_ws_request( char *buf, char *ws_request)
{
   int i,j;
   int a=0, b=0;

   /* ' '�� ���������� �����س�����. ' '�κ��� �迭��ġ�� ����. */
   for(i=0;i<BUFFSIZE;i++)
   {
       ws_request[i] = buf[i]; 
       if( buf[i] == ' ' ) break;
   }
 
   /* '/'���ڰ� 3�� ���ö����� �迭�� �پ��Ѵ´�. 3���������� ��ġ����.*/
   while( b < 3 )
      if( buf[a++] == '/' ) 
         b++;
   /* ' '���ں��� '/'���ڰ� 3�� ���°������� �پ��Ѱ� �ٽ� �迭 ����.*/
   for(j=0;j<BUFFSIZE-i;j++)
   {
      ws_request[j+i+1] = buf[a+j-1];
      if( buf[j] == '\0')
         break;
   }
} 


static void  my_alarm( int signo )
{
   if( signal(SIGALRM, my_alarm) ==  SIG_ERR)
      perror("signal error");
   close(accept_sd);
   close(connect_sd);
   printf("Alarm fucntion called.\n");
   exit(0);
}


static void  sig_cld( int signo )
{
   pid_t pid;
   int status;
   //printf("child terminated.\n");
   if( signal(SIGCHLD, sig_cld) == SIG_ERR )
      perror("signal error");
   if( (pid = wait(&status)) < 0)
      perror("wait error");
   return;
}


/*************************************************************************
�߰� ���� �Լ��μ� hostent �ּ� ����ü�� ���ϵ� �����Ͱ��� ���� ���� ��, ����
���� �ʴ� url�� �Է����� ���������� (proxy server����) ���Ƿ� �������������� 
������ ���������� ������.
**************************************************************************/ 
void errpage(char *url)
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
