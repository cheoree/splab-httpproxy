#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>  
#include <strings.h>      //bzero()
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>        //hostent 구조체
#include <errno.h>
 
#define BUFFSIZE              4096 
#define MAXLINE               4096 
#define DNSIZE                 256   //도메인네임 사이즈 
#define MAX_NUM_CLIENT           5
#define PORTNUM              61003 
#define WEBPORT                 80
#define TIMEOUT                120 

int listen_sd, accept_sd, connect_sd;  

int          IS_HIT( char *, char *, char *, char * );   
char       * GET_HASH_URL( char * );
void         CACHEING( char *, char * );
void         CONNECT_WEB_SERV( char *, char * );
void         ACCESS_LOCAL_CACHE( char * );
void         GET_SHORT_REQUEST(char *, char *);
void         GET_WS_REQUEST(char *, char *); 
void         ERRPAGE(char *);




#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
   If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
     returns an empty list.
      */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

/* MD5 context. */
typedef struct {
      UINT4 state[4];                                   /* state (ABCD) */
        UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
          unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

void MD5Init PROTO_LIST ((MD5_CTX *));
void MD5Update PROTO_LIST
  ((MD5_CTX *, unsigned char *, unsigned int));
  void convert_md5( char *it, char *val );
  void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX *));

