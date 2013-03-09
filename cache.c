/**************************************************** 
ASSIGNMENT #3 :  Proxy Cache Server                                                               
NAME          :  ��ö��                                                                           
                                                                      
DATE          :  2000. 12. 1                                                                      
COMPILER      :  UNIX gcc 2.8.1,  LINUX gcc 2.9.0                                                 
FILE          :  socket.c,  cache.c,  md5.c,  proxy.h                                             
*****************************************************/

/*************   cache.c  ***********************************************************************/
#include "proxy.h"


void  cacheing( char *URL, char *buf )
{
    int  qlog; 
    
    int x,         i = 1900;                     //Ÿ���Լ������� ���� ���� ������.
    time_t             Time;                     //
    struct tm            *T;                     //

    char dir0[6]  =      "cache";           // ./cache
    char dir1[8]  =      "cache";           // ./cache/x
    char dir2[10] =      "cache";           // ./cache/x/y
    char whole_dir[35] = "cache";           // ./cache/x/y/zabcdefg...
    
    char    log_buf[128] = {'\0'};           // sprintf�� ������ ���� ĳ���� ����
    char  hfile_buf[128] = {'\0'};           // �ϴ� NULL�� �ʱ�ȭ�Ͽ����ϴ�.
    char hashed_url[128] = {'\0'};       //hash�Ǿ� ���� ����.

    time( &Time );
    T = localtime( &Time );

    convert_md5( URL, hashed_url );  //url -> hashed_url ��ȯ
    
    dir1[5] =           '/';         //ù��° �����丮("cache/x")
    dir1[6] = hashed_url[0];         //cache...������ ���� �κ��̹Ƿ�
    dir1[7] =          '\0';         //5��° �������� ���Խ����Ѵ�.

    dir2[5] =           '/';      
    dir2[6] = hashed_url[0];         //ù��°�� �̾� �ι�° �����丮���� 
    dir2[7] = hashed_url[1];         //�̾� ����.("cache/x/y")
    dir2[8] = hashed_url[2];
    dir2[9] =          '\0'; 

    whole_dir[5] = '/';        
    for( x=0;x<28;x++) whole_dir[x+6] = hashed_url[x]; 
    whole_dir[34] = '\0';          

    qlog = open("log", O_WRONLY | O_CREAT | O_APPEND, 0666 ); 
            
    if( is_hit( dir0, dir1, dir2, whole_dir ) == 0 )     // MISS
    {
        sprintf( log_buf, "%s - [%d/%d/%d %d:%d:%d]\n", URL, i+T->tm_year,
           T->tm_mon+1, T->tm_mday, T->tm_hour, T->tm_min, T->tm_sec );
        write( qlog, log_buf, 128 );

        connect_web_srv(buf, whole_dir);
    }

    else    	                                         // HIT
    {
        sprintf( hfile_buf, "%s - [%d/%d/%d %d:%d:%d]\n", hashed_url, 
            i+T->tm_year, T->tm_mon+1, T->tm_mday, T->tm_hour, T->tm_min,
            T->tm_sec );
        write( qlog, hfile_buf, 128 ); 	    
              
        access_local_cache(whole_dir); 
    }

    close(qlog);

}

int is_hit( char *dir0, char *dir1, char *dir2, char *whole_dir )
{
    struct dirent                                 //�����丮�� �б����� ������ ���ǵ� 
              *f_cur_dirir,                              //����ü�� ������ ������.
       	      *f_dir0, 
              *f_dir1, 
	      *f_dir2;

    DIR       *d_cur_dirir = NULL;
    DIR       *d_dir0 = NULL;
    DIR       *d_dir1 = NULL;
    DIR       *d_dir2 = NULL;

    int n, is_empty, tmp,  i = 0;
    int flgC =    0;                    //�����丮 ���翩�θ� ������ flag��.
    int flg0 =    0;                    // �ʱⰪ�� 0���� ����.
    int flg1 =    0;                    // cache, dir1, dir2�� �б����Ͽ� 3��.

    char *temp;
    char *cur_dir = ".";                  // <- ������ �����丮 (.)
    char first_dir[2]       = {'\0'};   // ������ ������ cache���� �������θ��� 
    char second_dir[2]      = {'\0'};   // �ƴ� �װ͵��� �����Ͽ� �˻��Ǿ��� 
    char last_file_name[25] = {'\0'};   // �������� �����丮 �̸��� ������ ������.

    first_dir[0]  = whole_dir[6];  //��ü ���θ��� 6��° ������ �ٷ� ù��° ���� 
    second_dir[0] = whole_dir[8];  //�����丮�� �̸�. �ι�°�� ��������. 
    for( n=0; n<24; n++ ) last_file_name[n] = whole_dir[n+10]; //������, �����̸�.

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
	                          i=1;     // �������� ������ �����Դٸ� �Լ��� Hit           
		          }                // ���� �ǹ̷� �Ʒ����� 1�� �����Ѵ�.
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
    
    /* �÷��� ���������� cache���� �����丮�� ������. �����丮�� 
    �����ϴ� ���� ���� �ٽ� ������ �ʴ´�.*/
    if( flgC == 0 ) mkdir( dir0, S_IRWXU | S_IRGRP ); 
    if( flg0 == 0 ) mkdir( dir1, S_IRWXU | S_IRGRP );
    if( flg1 == 0 ) mkdir( dir2, S_IRWXU | S_IRGRP );  
    
    return i;
}
/*************   End of File   **************************************************************/
