#!/bin/sh

INDENT_PROGRAM="uncrustify"
DIR="tools"
CFG="cheese-indent.cfg"
LANG="VALA"

[[ $# -lt 1 ]] && { echo "$0 [files]" 1>&2; exit 1; }

if ! which $INDENT_PROGRAM > /dev/null 2>&1; then
  echo "$INDENT_PROGRAM was not found on your computer, please install it"
  exit 1
fi

$INDENT_PROGRAM -l $LANG -c $DIR/$CFG --no-backup --replace $@
