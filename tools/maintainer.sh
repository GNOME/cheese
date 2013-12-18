#!/bin/sh

# this is a small helper script for preparing the cheese releases

PACKAGE_NAME="Cheese"
PACKAGE_NEWS_FILE="NEWS"
PACKAGE_ABOUT="Take photos and videos with your webcam, with fun graphical effects"
PACKAGE_WEBSITE="https://wiki.gnome.org/Apps/Cheese/"
GNOME_SERVER="http://download.gnome.org/sources/"

TO="gnome-announce-list@gnome.org"

###############################################
###############################################
###############################################

if [ ! -d ".git" ]; then
  echo "`pwd` is not a valid git repository"
  exit
fi

PACKAGE_MODULE=$(echo $PACKAGE_NAME | tr "[:upper:]" "[:lower:]")
PACKAGE_SCREENSHOTS="${PACKAGE_WEBSITE}Tour"
PACKAGE_VERSION=$(cat $PACKAGE_NEWS_FILE | grep -m1 version | awk '{ print $2 }')
#PACKAGE_VERSION=$(cat configure.ac | grep AC_INIT | awk '{print $2}' | sed "s/)//")
PACKAGE_VERSION=$(cat configure.ac | grep -A 1 AC_INIT | grep -o "[0-9]\.[0-9]\{1,2\}\.[0-9]\{1,2\}")
SUBJECT="ANNOUNCE: $PACKAGE_NAME $PACKAGE_VERSION released"

###############################################
###############################################
###############################################

show_help() {
  echo "OPTIONS"
  echo "  -n --prepare-news [TAG]     - prepare news file"
  echo "  -l --list-changes [TAG]     - list the changes"
  echo "  -t --list-translators [TAG] - list the translators"
  echo "  -m --release-mail           - create the announcement mail"
  echo "  -h --help                   - show help (this)"
  exit
}

check_tag() {
  if [ -z $1 ]; then
    echo "TAG missing, please specify the tag of the last release"
    echo
    echo "Choose any of the tags listed below"
    git tag -l | column
    exit
  else
    TAG=$1
  fi
}

create_release_mail() {
  IFS=. echo "$PACKAGE_VERSION" | read one two three
  PACKAGE_DOWNLOAD="$GNOME_SERVER$PACKAGE_MODULE/$one.$two/"

  NEWS_LINE_BEGIN=`expr 1 + $(grep -m2 -n "version [0-9]\+.[0-9]\+" $PACKAGE_NEWS_FILE | grep $PACKAGE_VERSION | sed "s/:.*$//")`
  NEWS_LINE_END=`expr -2 + $(grep -m2 -n "version [0-9]\+.[0-9]\+" $PACKAGE_NEWS_FILE | grep -v $PACKAGE_VERSION | sed "s/:.*$//")`
  PACKAGE_NEWS=$(sed "$NEWS_LINE_BEGIN,$NEWS_LINE_END!d" $PACKAGE_NEWS_FILE)

TEMPLATE="


what is it?
===========
$PACKAGE_ABOUT

what's changed in $PACKAGE_VERSION?
=========================
$PACKAGE_NEWS

where can i get it?
===================
you can get it by pressing here!
$PACKAGE_DOWNLOAD

what does it look like?
=======================
take a look here!
$PACKAGE_SCREENSHOTS

where can i find out more?
==========================
you can visit the project web site:
$PACKAGE_WEBSITE


"

  URL="mailto:$TO?subject=$SUBJECT&body=$TEMPLATE"
  xdg-open "$URL"
}


diff_files() {
  git diff $TAG..HEAD --name-only -- $1 | grep "\.po$"
}

list_translators() {
  PO_FILES=$(diff_files po)
  HELP_FILES=$(diff_files help)

  echo "  - Added/Updated Translations"
  (
  for i in $PO_FILES; do
    echo "    - $(basename $i .po), courtesy of $(grep "Last-Translator" $i | sed -e 's/"Last-Translator:  *\(.*\)  *<.*/\1/')"
  done
  ) | sort | uniq
  echo "  - Added/Updated Documentation"
  (
  for i in $HELP_FILES; do
    echo "    - $(basename $i .po), courtesy of $(grep "Last-Translator" $i | sed -e 's/"Last-Translator:  *\(.*\)  *<.*/\1/')"
  done
  ) | sort | uniq
}

list_changes() {
  git log $TAG.. --pretty="format:  - %s%n%b" --no-color | \
    fmt --split-only | \
    sed "s/^ *\([A-Za-z]\)/    \1/"
}

case "$1" in
     "-n"|"--prepare-news")
            check_tag $2
            TMPFILE="/tmp/news_tmp_maintainer"
            echo "version $PACKAGE_VERSION" > $TMPFILE
            echo "getting changes"
            list_changes >> $TMPFILE
            echo "getting translations"
            list_translators >> $TMPFILE
            echo >> $TMPFILE
            vim $PACKAGE_NEWS_FILE -c ":0" -c "/version" -c ":-1" -c ":r $TMPFILE" -c ":set nohlsearch"
            rm $TMPFILE
            exit
            ;;
     "-m"|"--release-mail")
            create_release_mail
            exit
            ;;
     "-l"|"--list-changes")
            check_tag $2
            list_changes $2
            exit
            ;;
     "-t"|"--list-translators")
            check_tag $2
            list_translators $2
            exit
            ;;
     "-h"|"help")
            show_help
            ;;
     *)
            show_help
            ;;
esac
