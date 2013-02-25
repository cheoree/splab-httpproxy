/**************************************************** 
ASSIGNMENT #3 :  Proxy Cache Server                                                               
NAME          :  배철희                                                                           
                                                                      
DATE          :  2000. 12. 1                                                                      
COMPILER      :  UNIX gcc 2.8.1,  LINUX gcc 2.9.0                                                 
FILE          :  socket.c,  cache.c,  md5.c,  proxy.h                                             
*****************************************************/

/*************   cache.c  ***********************************************************************/
#include "proxy.h"


void  CACHEING( char *URL, char *buf )
{
    int  qlog; 
    
    int x,         i = 1900;                     //타임함수사용을 위한 변수 선언들.
    time_t             Time;                     //
    struct tm            *T;                     //

    char dir0[6]  =      "cache";           // ./cache
    char dir1[8]  =      "cache";           // ./cache/x
    char dir2[10] =      "cache";           // ./cache/x/y
    char whole_dir[35] = "cache";           // ./cache/x/y/zabcdefg...
    
    char    log_buf[128] = {'\0'};           // sprintf에 사용될 버퍼 캐릭터 변수
    char  hfile_buf[128] = {'\0'};           // 일단 NULL로 초기화하였습니다.
    char hashed_url[128] = {'\0'};       //hash되어 나올 변수.

    time( &Time );
    T = localtime( &Time );

    convert_md5( URL, hashed_url );  //url -> hashed_url 변환
    
    dir1[5] =           '/';         //첫번째 디렉토리("cache/x")
    dir1[6] = hashed_url[0];         //cache...다음에 오는 부분이므로
    dir1[7] =          '\0';         //5번째 변위부터 대입시작한다.

    dir2[5] =           '/';      
    dir2[6] = hashed_url[0];         //첫번째에 이어 두번째 디렉토리명을 
    dir2[7] = hashed_url[1];         //이어 붙임.("cache/x/y")
    dir2[8] = hashed_url[2];
    dir2[9] =          '\0'; 

    whole_dir[5] = '/';        
    for( x=0;x<28;x++) whole_dir[x+6] = hashed_url[x]; 
    whole_dir[34] = '\0';          

    qlog = open("log", O_WRONLY | O_CREAT | O_APPEND, 0666 ); 
            
    if( IS_HIT( dir0, dir1, dir2, whole_dir ) == 0 )     // MISS
    {
        sprintf( log_buf, "%s - [%d/%d/%d %d:%d:%d]\n", URL, i+T->tm_year,
           T->tm_mon+1, T->tm_mday, T->tm_hour, T->tm_min, T->tm_sec );
        write( qlog, log_buf, 128 );

        CONNECT_WEB_SERV(buf, whole_dir);
    }

    else    	                                         // HIT
    {
        sprintf( hfile_buf, "%s - [%d/%d/%d %d:%d:%d]\n", hashed_url, 
            i+T->tm_year, T->tm_mon+1, T->tm_mday, T->tm_hour, T->tm_min,
            T->tm_sec );
        write( qlog, hfile_buf, 128 ); 	    
              
        ACCESS_LOCAL_CACHE(whole_dir); 
    }

    close(qlog);

}

int IS_HIT( char *dir0, char *dir1, char *dir2, char *whole_dir )
{
    struct dirent                                 //디렉토리를 읽기위한 헤더에 정의된 
              *f_cur_dirir,                              //구조체와 포인터 변수들.
       	      *f_dir0, 
              *f_dir1, 
	      *f_dir2;

    DIR       *d_cur_dirir = NULL;
    DIR       *d_dir0 = NULL;
    DIR       *d_dir1 = NULL;
    DIR       *d_dir2 = NULL;

    int n, is_empty, tmp,  i = 0;
    int flgC =    0;                    //디렉토리 존재여부를 가리는 flag들.
    int flg0 =    0;                    // 초기값은 0으로 설정.
    int flg1 =    0;                    // cache, dir1, dir2를 읽기위하여 3개.

    char *temp;
    char *cur_dir = ".";                  // <- 현재의 디렉토리 (.)
    char first_dir[2]       = {'\0'};   // 위에서 설명한 cache기준 절대경로명이 
    char second_dir[2]      = {'\0'};   // 아닌 그것들과 비교하여 검색되어질 
    char last_file_name[25] = {'\0'};   // 개별적인 디렉토리 이름을 저장할 변수들.

    first_dir[0]  = whole_dir[6];  //전체 경로명의 6번째 변위는 바로 첫번째 하위 
    second_dir[0] = whole_dir[8];  //디렉토리의 이름. 두번째도 마찬가지. 
    for( n=0; n<24; n++ ) last_file_name[n] = whole_dir[n+10]; //마지막, 파일이름.

    d_cur_dirir = opendir(cur_dir);
    for( f_cur_dirir = readdir(d_cur_dirir); f_cur_dirir; f_cur_dirir = readdir(d_cur_dirir) )
    {
        if( strcmp( f_cur_dirir->d_name, dir0) == 0 ) 
        {
            flgC = 1;
            d_dir0 = opendir(dir0);
            for( f_dir0=readdir(d_dir0); f_dir0; f_dir0=readdir(d_dir0) )
            {
	        if( strcmp(f_dir0->d_name, first_dir) == 0 )
		{ 
                    flg0 = 1; 
	    	    d_dir1 = opendir(dir1); 
	            for( f_dir1=readdir(d_dir1); f_dir1; f_dir1=readdir(d_dir1) )
	            {
		       if( strcmp(f_dir1->d_name, second_dir) == 0 )
		       {
                          flg1 = 1;
		          d_dir2 = opendir(dir2);
		          for( f_dir2=readdir(d_dir2); f_dir2; f_dir2=readdir(d_dir2) )
		          {
                              is_empty = open(whole_dir, O_RDONLY);
                              tmp = read(is_empty, temp, 1024);
			      if( strcmp(f_dir2->d_name, last_file_name) == 0 && tmp!= 0)
	                          i=1;     // 여기까지 힘겹게 들어왔다면 함수는 Hit           
		          }                // 라는 의미로 아래에서 1을 리턴한다.
		          closedir(d_dir2);
		       }
	            }
	            closedir(d_dir1);
	        }
            }
            closedir(d_dir0);
        }
    }
    closedir(d_cur_dirir);
    
    /* 플래그 설정에따라 cache하위 디렉토리를 만든다. 디렉토리가 
    존재하는 것은 결코 다시 만들지 않는다.*/
    if( flgC == 0 ) mkdir( dir0, S_IRWXU | S_IRGRP ); 
    if( flg0 == 0 ) mkdir( dir1, S_IRWXU | S_IRGRP );
    if( flg1 == 0 ) mkdir( dir2, S_IRWXU | S_IRGRP );  
    
    return i;
}
/*************   End of File   **************************************************************/
