dnl Checks the location of the XML Catalog
dnl Usage:
dnl	JH_PATH_XML_CATALOG([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl Defines XMLCATALOG and XML_CATALOG_FILE substitutions
AC_DEFUN([JH_PATH_XML_CATALOG],
[
	dnl check for the presence of the XML catalog
	AC_ARG_WITH([xml-catalog],
		AS_HELP_STRING([--with-xml-catalog=CATALOG],
		[path to xml catalog to use]),,
		[with_xml_catalog=''])
	AC_MSG_CHECKING([for XML catalog])
	if test -n "$with_xml_catalog"; then
		dnl path was explicitly given.  check that it exists.
		if test -f "$with_xml_catalog"; then
			jh_found_xmlcatalog=true
		else
			jh_found_xmlcatalog=false
		fi
	else
		dnl if one was not explicitly specified, try some guesses
		dnl we look first in /etc/xml/catalog, then XDG_DATA_DIRS/xml/catalog
		if test -z "$XDG_DATA_DIRS"; then
			dnl if we have no XDG_DATA_DIRS, use the default
			jh_xml_catalog_searchdirs="/etc:/usr/local/share:/usr/share"
		else
			jh_xml_catalog_searchdirs="/etc:$XDG_DATA_DIRS"
		fi
		jh_found_xmlcatalog=false
		dnl take care to iterate based on ':', allowing whitespace to appear in paths
		jh_xml_catalog_saved_ifs="$IFS"
		IFS=':'
		for d in $jh_xml_catalog_searchdirs; do
			if test -f "$d/xml/catalog"; then
				with_xml_catalog="$d/xml/catalog"
				jh_found_xmlcatalog=true
				break
			fi
		done
		IFS="$jh_xml_catalog_saved_ifs"
	fi
	if $jh_found_xmlcatalog; then
		AC_MSG_RESULT([$with_xml_catalog])
	else
		AC_MSG_RESULT([not found])
	fi
	XML_CATALOG_FILE="$with_xml_catalog"
	AC_SUBST([XML_CATALOG_FILE])

	dnl check for the xmlcatalog program
	AC_PATH_PROG(XMLCATALOG, xmlcatalog, no)
	if test "x$XMLCATALOG" = xno; then
		jh_found_xmlcatalog=false
	fi

	if $jh_found_xmlcatalog; then
		ifelse([$1],,[:],[$1])
	else
		ifelse([$2],,[AC_MSG_ERROR([could not find XML catalog])],[$2])
	fi
])
