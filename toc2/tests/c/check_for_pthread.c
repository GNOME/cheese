#include <pthread.h>
// link with -lpthread
#include <stdio.h>

void * thread_callback( void * arg )
{
  printf( "Doing nothing.\n" );
}
int main()
{
  pthread_t thread = 0;
  int thret = pthread_create( &thread, NULL, thread_callback, NULL );
  return 0;
}
