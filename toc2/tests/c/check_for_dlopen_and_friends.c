/**
   Quick check for dlopen(), dlclose() and dlsym().

   toc usage:

   toc_test_require path/to/this/file -ldl -export-dynamic
 */
#include <dlfcn.h>
#include <stdlib.h>
void foo_function() {}

int main()
{
        typedef void (*func)();
        void * soh = dlopen( 0, RTLD_NOW | RTLD_GLOBAL );
        if( ! soh )
        {
                printf( "%s\n", dlerror() );
                return 1;
        }
        void * sym = (func) dlsym( soh, "foo_function" );
        if( 0 == sym )
        {
                printf( "%s\n", dlerror() );
                return 2;
        }
        int err = dlclose( soh );
        if( err )
        {
                printf( "%s\n", dlerror() );
                return err;
        }
        return 0;
}
