/**
   Quick check for lt_dlopen(), lt_dlclose() and lt_dlsym().

   toc usage:

   toc_test_require path/to/this/file -lltdl -export-dynamic
 */
#include <ltdl.h>
#include <stdlib.h>
void foo_function() {}

int main()
{
        typedef void (*func)();
        void * soh = 0;
        lt_dlinit();
        soh = lt_dlopen( 0 ); // , RTLD_NOW | RTLD_GLOBAL );
        if( ! soh )
        {
                printf( "could not open main app: %s\n", lt_dlerror() );
                return 1;
        }
        void * sym = (func) lt_dlsym( soh, "foo_function" );
        if( 0 == sym )
        {
                printf( "could not find test symbol: %s\n", lt_dlerror() );
                return 2;
        }
        return 0;
}
