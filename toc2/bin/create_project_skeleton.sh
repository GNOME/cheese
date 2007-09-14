#!/bin/bash
########################################################################
# A quick hack for generating a skeleton project for use with toc2.
#
# Must be run from the top of the toc2 source tree.
#
#

test -f ./toc2/sbin/toc2_core.sh -a -f ./configure.toc2 || {
    echo "$0 must be run from the top of the toc2 source tree."
    exit 1
}

test -e ./toc2.toc2.make || {
    echo "Configuring ..."
    ./configure >/dev/null || {
	echo "configure failed :("
	exit 2
    }
}

echo "Making dist tarball..."
make dist > /dev/null || {
    echo "make dist failed :("
    exit 3
}

tarball=$(ls -1t toc2-*.tar | head -n1)

test x != "x${tarball}" || {
    echo "Could not figure out which file is the tarball??? :("
    exit 4
}

distname=${tarball%%.tar}
test x = "x${distname}" && {
    echo "Could not determine tarball name???"
    exit 5
}

test -d ${distname} && rm -fr ${distname}

tar xf ${tarball}

cd $distname || {
    echo "Could not cd to [$distname]???"
    exit 6
}
rm -fr src
rm -fr toc2/doc
skelname=ProjectSkeleton
perl -i -pe "s|PACKAGE_NAME=\S+|PACKAGE_NAME=${skelname}|" configure
mv configure.toc2 configure.${skelname}
mv toc2.toc2.make.at toc2.${skelname}.make.at
mv Makefile.skeleton Makefile

cd -

version=${distname##*-}
newname=toc2-skeleton-package-${version}
mv ${distname} ${newname}
newtar=${newname}.tar.gz
tar czf ${newtar} ${newname}
rm -fr ${newname}

echo "Done..."
ls -lat ${newtar}
