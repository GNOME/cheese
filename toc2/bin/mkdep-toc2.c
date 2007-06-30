/*
 * Originally by Linus Torvalds.
 * Smart CONFIG_* processing by Werner Almesberger, Michael Chastain.
 *
 * Usage: mkdep cflags -- file ...
 * 
 * Read source files and output makefile dependency lines for them.
 * I make simple dependency lines for #include <*.h> and #include "*.h".
 * I also find instances of CONFIG_FOO and generate dependencies
 *    like include/config/foo.h.
 *
 * 1 August 1999, Michael Elizabeth Chastain, <mec@shout.net>
 * - Keith Owens reported a bug in smart config processing.  There used
 *   to be an optimization for "#define CONFIG_FOO ... #ifdef CONFIG_FOO",
 *   so that the file would not depend on CONFIG_FOO because the file defines
 *   this symbol itself.  But this optimization is bogus!  Consider this code:
 *   "#if 0 \n #define CONFIG_FOO \n #endif ... #ifdef CONFIG_FOO".  Here
 *   the definition is inactivated, but I still used it.  It turns out this
 *   actually happens a few times in the kernel source.  The simple way to
 *   fix this problem is to remove this particular optimization.
 *
 * 2.3.99-pre1, Andrew Morton <andrewm@uow.edu.au>
 * - Changed so that 'filename.o' depends upon 'filename.[cS]'.  This is so that
 *   missing source files are noticed, rather than silently ignored.
 *
 * 2.4.2-pre3, Keith Owens <kaos@ocs.com.au>
 * - Accept cflags followed by '--' followed by filenames.  mkdep extracts -I
 *   options from cflags and looks in the specified directories as well as the
 *   defaults.   Only -I is supported, no attempt is made to handle -idirafter,
 *   -isystem, -I- etc.
 *
 * Aug 2003: stephan beal <stephan@s11n.net>
 * - removed HPATH requirement for use in the toc project (toc.sourceforge.net)
 *
 * Aug 2003: rusty <bozo@users.sourceforge.net>
 * Other changes for use in toc/libfunUtil:
 * - Add support for C++ file extensions .cc, .C, .c++, .cxx, .cpp.
 *   (Previously, this was doing "a" right thing, by making foo.c++ depend on
 *   the headers #included by foo.c++, with a "touch foo.c++" rule which
 *   would then cause the .o file to be out of date, but "the" right thing is
 *   for foo.o to depend on the headers #included by foo.c++.  I guess there
 *   aren't many C++ files in the Linux kernel tree, ha ha.)
 *
 * Dec 2003: stephan@s11n.net
 * Changes for toc.sourceforge.net:
 * - removed the default 'touch' behaviour to avoid collissions with targets
 *   generated via toc.
 *
 * 20 Aug 2004: stephan@s11n.net
 * - Removed unused hpath from main(). WTF does gcc NOW start to
 * complain about that!?!?!?
 * - Added some parens to keep gcc from bitching (again... why now? 
 * Been compiling w/o complaint for over a year).
 *
 * 7 April 2005: stephan@s11n.net
 * - Made an -Isomedir dir lookup failure non-fatal, because it often
 * happens when using a custom --prefix when configuring a tree to
 * build under, e.g. ~/tmp.
 *
 * 8 June 2007: stephan@s11n.net
 * - Added creation of empty target for files which have no explicit deps.
 * This avoids the "no rule to create ..." when a file is moved/deleted.*
 *
 * 16 June 2007: stephan@s11n.net
 * - Removed all CONFIG-related code (~150 lines), as it is not useful
 * outside of the linux kernel (original home of this code) and it
 * greatly decreases the readability of this code.
 *
 */

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


/**
   TOC_MODE enables:

   - no 'touch'ing of targets, to avoid overriding
   rules defined elsewhere in toc.
*/
#define TOC_MODE 1

char __depname[512] = "\n\t@touch ";
#define depname (__depname+9)

int hasdep;

struct path_struct {
	int len;
	char *buffer;
};
struct path_struct *path_array;
int paths;


/* Current input file */
static const char *g_filename;

/*
 * This records all the configuration options seen.
 * In perl this would be a hash, but here it's a long string
 * of values separated by newlines.  This is simple and
 * extremely fast.
 */
char * str_config  = NULL;
int    size_config = 0;
int    len_config  = 0;

static void
do_depname(void)
{
	if (!hasdep) {
		hasdep = 1;
                //printf( "depname=%s, g_filename=%s", depname, g_filename );
		printf("%s:", depname);
		if (g_filename)
			printf(" %s", g_filename);
	}
}


/*
 * This records all the precious .h filenames.  No need for a hash,
 * it's a long string of values enclosed in tab and newline.
 */
char * str_precious  = NULL;
int    size_precious = 0;
int    len_precious  = 0;



/*
 * Grow the precious string to a desired length.
 * Usually the first growth is plenty.
 */
void grow_precious(int len)
{
	while (len_precious + len > size_precious) {
		if (size_precious == 0)
			size_precious = 2048;
		str_precious = realloc(str_precious, size_precious *= 2);
		if (str_precious == NULL)
			{ perror("malloc"); exit(1); }
	}
}



/*
 * Add a new value to the precious string.
 */
void define_precious(const char * filename)
{
	int len = strlen(filename);
	grow_precious(len + 4);
	*(str_precious+len_precious++) = '\t';
	memcpy(str_precious+len_precious, filename, len);
	len_precious += len;
	memcpy(str_precious+len_precious, " \\\n", 3);
	len_precious += 3;
}



/*
 * Handle an #include line.
 */
void handle_include(int start, const char * name, int len)
{
	struct path_struct *path;
	int i;

	for (i = start, path = path_array+start; i < paths; ++i, ++path) {
		memcpy(path->buffer+path->len, name, len);
		path->buffer[path->len+len] = '\0';
		if (access(path->buffer, F_OK) == 0) {
			do_depname();
			printf(" \\\n   %s", path->buffer);
			return;
		}
	}

}



/*
 * Add a path to the list of include paths.
 */
void add_path(const char * name)
{
	struct path_struct *path;
	char resolved_path[PATH_MAX+1];
	const char *name2;

	if (strcmp(name, ".")) {
		name2 = realpath(name, resolved_path);
		if (!name2) {
			return; /* added by stephan@s11n.net, because i don't care about this failure. */
/* 			fprintf(stderr, "realpath(%s) failed, %m\n", name); */
/* 			exit(1); */
		}
	}
	else {
		name2 = "";
	}

	path_array = realloc(path_array, (++paths)*sizeof(*path_array));
	if (!path_array) {
		fprintf(stderr, "cannot expand path_arry\n");
		exit(1);
	}

	path = path_array+paths-1;
	path->len = strlen(name2);
	path->buffer = malloc(path->len+1+256+1);
	if (!path->buffer) {
		fprintf(stderr, "cannot allocate path buffer\n");
		exit(1);
	}
	strcpy(path->buffer, name2);
	if (path->len && *(path->buffer+path->len-1) != '/') {
		*(path->buffer+path->len) = '/';
		*(path->buffer+(++(path->len))) = '\0';
	}
}





/*
 * Macros for stunningly fast map-based character access.
 * __buf is a register which holds the current word of the input.
 * Thus, there is one memory access per sizeof(unsigned long) characters.
 */

#if defined(__alpha__) || defined(__i386__) || defined(__ia64__)  || defined(__x86_64__) || defined(__MIPSEL__)	\
    || defined(__arm__)
#define LE_MACHINE
#endif

#ifdef LE_MACHINE
#define next_byte(x) (x >>= 8)
#define current ((unsigned char) __buf)
#else
#define next_byte(x) (x <<= 8)
#define current (__buf >> 8*(sizeof(unsigned long)-1))
#endif

#define GETNEXT { \
	next_byte(__buf); \
	if ((unsigned long) next % sizeof(unsigned long) == 0) { \
		if (next >= end) \
			break; \
		__buf = * (unsigned long *) next; \
	} \
	next++; \
}

/*
 * State machine macros.
 */
#define CASE(c,label) if (current == c) goto label
#define NOTCASE(c,label) if (current != c) goto label

/*
 * Yet another state machine speedup.
 */
#define MAX2(a,b) ((a)>(b)?(a):(b))
#define MIN2(a,b) ((a)<(b)?(a):(b))
#define MAX4(a,b,c,d) (MAX2(a,MAX2(b,MAX2(c,d))))
#define MIN4(a,b,c,d) (MIN2(a,MIN2(b,MIN2(c,d))))



/*
 * The state machine looks for (approximately) these Perl regular expressions:
 *
 *    m|\/\*.*?\*\/|
 *    m|\/\/.*|
 *    m|'.*?'|
 *    m|".*?"|
 *    m|#\s*include\s*"(.*?)"|
 *    m|#\s*include\s*<(.*?>"|
 *
 * About 98% of the CPU time is spent here, and most of that is in
 * the 'start' paragraph.  Because the current characters are
 * in a register, the start loop usually eats 4 or 8 characters
 * per memory read.  The MAX4 and MIN4 tests dispose of most
 * input characters with 1 or 2 comparisons.
 */
void state_machine(const char * map, const char * end)
{
	const char * next = map;
	const char * map_dot;
	unsigned long __buf = 0;

	for (;;) {
start:
	GETNEXT
__start:
	if (current > MAX4('/','\'','"','#')) goto start;
	if (current < MIN4('/','\'','"','#')) goto start;
	CASE('/',  slash);
	CASE('\'', squote);
	CASE('"',  dquote);
	CASE('#',  pound);
	goto start;

/* // */
slash_slash:
	GETNEXT
	CASE('\n', start);
	NOTCASE('\\', slash_slash);
	GETNEXT
	goto slash_slash;

/* / */
slash:
	GETNEXT
	CASE('/',  slash_slash);
	NOTCASE('*', __start);
slash_star_dot_star:
	GETNEXT
__slash_star_dot_star:
	NOTCASE('*', slash_star_dot_star);
	GETNEXT
	NOTCASE('/', __slash_star_dot_star);
	goto start;

/* '.*?' */
squote:
	GETNEXT
	CASE('\'', start);
	NOTCASE('\\', squote);
	GETNEXT
	goto squote;

/* ".*?" */
dquote:
	GETNEXT
	CASE('"', start);
	NOTCASE('\\', dquote);
	GETNEXT
	goto dquote;

/* #\s* */
pound:
	GETNEXT
	CASE(' ',  pound);
	CASE('\t', pound);
	CASE('i',  pound_i);
	goto __start;

/* #\s*i */
pound_i:
	GETNEXT NOTCASE('n', __start);
	GETNEXT NOTCASE('c', __start);
	GETNEXT NOTCASE('l', __start);
	GETNEXT NOTCASE('u', __start);
	GETNEXT NOTCASE('d', __start);
	GETNEXT NOTCASE('e', __start);
	goto pound_include;

/* #\s*include\s* */
pound_include:
	GETNEXT
	CASE(' ',  pound_include);
	CASE('\t', pound_include);
	map_dot = next;
	CASE('"',  pound_include_dquote);
	CASE('<',  pound_include_langle);
	goto __start;

/* #\s*include\s*"(.*)" */
pound_include_dquote:
	GETNEXT
	CASE('\n', start);
	NOTCASE('"', pound_include_dquote);
	handle_include(0, map_dot, next - map_dot - 1);
	goto start;

/* #\s*include\s*<(.*)> */
pound_include_langle:
	GETNEXT
	CASE('\n', start);
	NOTCASE('>', pound_include_langle);
	handle_include(1, map_dot, next - map_dot - 1);
	goto start;
    }
}



/*
 * Generate dependencies for one file.
 */
void do_depend(const char * filename, const char * command)
{
	int mapsize;
	int pagesizem1 = getpagesize()-1;
	int fd;
	struct stat st;
	char * map;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr,"cannot open file: %s\n",filename);
		return;
	}

	fstat(fd, &st);
	if (st.st_size == 0) {
		fprintf(stderr,"%s is empty\n",filename);
		close(fd);
		return;
	}

	mapsize = st.st_size;
	mapsize = (mapsize+pagesizem1) & ~pagesizem1;
	map = mmap(NULL, mapsize, PROT_READ, MAP_PRIVATE, fd, 0);
	if ((long) map == -1) {
		fprintf(stderr,"mmap failed\n");
		close(fd);
		return;
	}
	if ((unsigned long) map % sizeof(unsigned long) != 0)
	{
		fprintf(stderr, "do_depend: map not aligned\n");
		exit(1);
	}

	hasdep = 0;
	state_machine(map, map+st.st_size);
	if (hasdep) {
		puts(command);
		if (*command)
			define_precious(filename);
	}
	else
	{
		// Create empty target to avoid "no rule to create ..." when we delete/rename a file:
		printf("%s:\n", filename);
	}

	munmap(map, mapsize);
	close(fd);
}



/*
 * Generate dependencies for all files.
 */
int main(int argc, char **argv)
{
	int len;
	add_path(".");		/* for #include "..." */
	while (++argv, --argc > 0) {
		if (strncmp(*argv, "-I", 2) == 0) {
			if (*((*argv)+2) ) {
				add_path((*argv)+2);
			}
			else {
				++argv;
				--argc;
			}
		}
		else if (strcmp(*argv, "--") == 0) {
			break;
		}
	}

	while (--argc > 0) {
		const char * filename = *++argv;
#if TOC_MODE
		const char * command  = "";
#else
		const char * command  = __depname;
#endif
		g_filename = 0;
		len = strlen(filename);
		memcpy(depname, filename, len+1);
                if (len > 2 && filename[len-2] == '.') {
                        if (filename[len-1] == 'c' ||
                            filename[len-1] == 'S' ||
                            filename[len-1] == 'C') {
                                depname[len-1] = 'o';
                                g_filename = filename;
                                command = "";
                        }
                }
                else if (len > 3 && filename[len-3] == '.') {
                        if (filename[len-2] == 'c' && filename[len-1] == 'c') {
                                depname[len-2] = 'o';
                                depname[len-1] = '\0';
                                g_filename = filename;
                                command = "";
                        }
                }
                else if (len > 4 && filename[len-4] == '.') {
                        if ( filename[len-3] == 'c'  &&  // check for c++/cxx/cpp
                              (
                               (filename[len-2] == '+' && filename[len-1] == '+') ||
                               (filename[len-2] == 'x' && filename[len-1] == 'x') ||
                               (filename[len-2] == 'p' && filename[len-1] == 'p')
                               )
                             ) {
                                depname[len-3] = 'o';
                                depname[len-2] = '\0';
                                g_filename = filename;
                                command = "";
                        }
                }
		do_depend(filename, command);
	}
	if (len_precious) {
		*(str_precious+len_precious) = '\0';
		printf(".PRECIOUS:%s\n", str_precious);
	}
	return 0;
}
