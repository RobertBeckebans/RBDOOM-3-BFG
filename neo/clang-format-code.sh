#!/bin/sh

# keep the following in sync with our supplied clang-format binaries!
OUR_CLANGFMT_VERSION=11.0.0

print_usage () {
	#echo "By default, this script only works on Linux i686 or amd64 (with our supplied clang-format binaries)"
	#echo "You can use your own clang-format binary by setting the CLANGFMT_BIN environment variable before calling this script"
	#echo "  e.g.: CLANGFMT_BIN=/usr/bin/clang-format $0"
	echo "But please make sure it's version $OUR_CLANGFMT_VERSION because other versions may format differently!"
}

#if [ -z "$CLANGFMT_BIN" ]; then
#
#	if [ `uname -s` != "Linux" ]; then
#		print_usage
#		exit 1
#	fi

#	case "`uname -m`" in
#		i?86 | x86 ) CLANGFMT_SUFFIX="x86" ;;
#		amd64 | x86_64 ) CLANGFMT_SUFFIX="x86_64" ;;
#		* ) print_usage ; exit 1  ;;
#	esac

#	CLANGFMT_BIN="./clang-format.$CLANGFMT_SUFFIX"
#fi

CLANGFMT_BIN=clang-format

#CLANGFMT_VERSION=$($CLANGFMT_BIN --version | grep -o -e "[[:digit:]\.]*")
CLANGFMT_VERSION=$($CLANGFMT_BIN --version | egrep -o "([0-9]{1,}\.)+[0-9]{1,}")

if [ "$CLANGFMT_VERSION" != "$OUR_CLANGFMT_VERSION" ]; then
	echo "ERROR: $CLANGFMT_BIN has version $CLANGFMT_VERSION, but we want $OUR_CLANGFMT_VERSION"
	echo "       (Unfortunately, different versions of clang-format produce slightly different formatting.)"
	exit 1
fi

#clang-format -i renderer/RenderBackend.h
#clang-format -i renderer/RenderBackend.cpp

clang-format -i aas/AASFile.h
clang-format -i aas/*

#find . -regex ".*\.\(cpp\|cc\|cxx\|h\|hpp\)" ! -path "./libs/*" ! -path "./d3xp/gamesys/SysCvar.cpp" ! -path "./d3xp/gamesys/Callbacks.cpp" ! -path "./sys/win32/win_cpu.cpp" ! -path "./sys/win32_win_main.cpp" -exec $CLANGFMT_BIN -i {} \;

#find . -regex ".*\.\(c\|cpp\|cc\|cxx\|h\|hpp\)" ! -path "./libs/*" ! -exec $CLANGFMT_BIN -i {} \;
