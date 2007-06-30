#!/do/not/bash
# toc2_run_description = checking for expat XML parser...
# toc2_begin_help =
#
# Exports the variable HAVE_LIBEXPAT to 0 or 1.
# If --without-libexpat is passed to configure then this test
# sets HAVE_LIBEXPAT to 0 and returns success.
#
# = toc2_end_help



if [ x0 = "x${configure_with_libexpat}" ]; then
    echo "libexpat disabled explicitely with --without-libexpat."
    toc2_export HAVE_LIBEXPAT=0
    return 0
fi

src=check4expat.c
cat <<EOF > $src
// taken from expat's outline.c example, expat 1.95.7
#include <stdio.h>
#include <expat.h>

#define BUFFSIZE        8192

char Buff[BUFFSIZE];

int Depth;

static void XMLCALL
start(void *data, const char *el, const char **attr)
{}
static void XMLCALL
end(void *data, const char *el)
{}

int
main(int argc, char *argv[])
{
  XML_Parser p = XML_ParserCreate(NULL);
  if (! p) {
    fprintf(stderr, "Couldn't allocate memory for parser\n");
    exit(-1);
  }

  XML_SetElementHandler(p, start, end);
  return 0;
}

EOF

workie=1
toc2_test gcc_build_and_run $src -lexpat || workie=0

rm $src

toc2_export HAVE_LIBEXPAT=${workie}
unset workie
test 1 = ${HAVE_LIBEXPAT}
return $?
