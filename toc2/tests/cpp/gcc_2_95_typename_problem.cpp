#include <map>
using namespace std;

#ifndef TYPENAME
#define TYPENAME typename
// gcc 3.x accepts typename, gcc 2.95.3 does not
#endif

template <typename T>
struct Foo
{
        typedef T TT;
        typedef Foo<TT> ThisType;
        typedef map<ThisType::TT,ThisType::TT> map_type;

        /**
           gcc 2.95.3 will do something like the following:
           stl_check.cpp: In method `void Foo<T>::check_for_gcc2_9_failure(const map<T,T,less<_Key>,allocator<_Tp1> > &)':
           stl_check.cpp:15: no class template named `map' in `struct Foo<T>'
           stl_check.cpp:16: no class template named `map' in `struct Foo<T>'

           gcc 3.x (3.3pre, at least) swallows it nicely.

           The problem here is the 'typename' part: without that gcc
           2.95.3 compiles it fine. gcc 3, however, warns that
           "implicite typenames are deprecated", and this kills my
           builds (i always use -Werror :/).
        */
        void check_for_gcc2_9_failure()
        {
                ThisType::map_type map;
                TYPENAME ThisType::map_type::const_iterator it = map.begin();
                TYPENAME ThisType::map_type::const_iterator et = map.end();
                for( ; it != et; ++it ) {}
        }
};

int main()
{
        typedef Foo<int> Fii;
        Fii fii;
        fii.check_for_gcc2_9_failure();
        return 0;
}
