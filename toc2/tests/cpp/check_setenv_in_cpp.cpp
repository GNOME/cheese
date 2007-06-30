#include <stdlib.h>

/**
On the solaris boxes i have i can't compile this as c++, but can as c,
using gcc 2.95.3:

/opt/gnu/bin/gcc toc/tests/cpp/check_setenv_in_cpp.cpp -c -o .toc.try_compile.o
toc/tests/cpp/check_setenv_in_cpp.cpp: In function `int main()':
toc/tests/cpp/check_setenv_in_cpp.cpp:9: implicit declaration of function `int setenv(...)'

 */
int main()
{
  setenv( "foo", "foo", 1 );
  return 0;
}
