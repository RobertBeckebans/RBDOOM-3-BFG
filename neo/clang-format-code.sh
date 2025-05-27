#!/bin/sh

# Desired clang-format version
OUR_CLANGFMT_VERSION=18.1.8

print_usage () {
    echo "Please ensure that clang-format version $OUR_CLANGFMT_VERSION is used, as other versions may produce different formatting!"
    echo "Set the CLANGFMT_BIN environment variable to specify a custom path to clang-format."
    echo "Example: CLANGFMT_BIN=/usr/bin/clang-format $0"
}

# Determine platform and set clang-format binary path
case "$(uname -s)" in
    Linux*)   CLANGFMT_DEFAULT="./clang-format" ;;
    MINGW*|MSYS*|CYGWIN*) CLANGFMT_DEFAULT="./clang-format.exe" ;;
    *)        echo "ERROR: Unsupported platform: $(uname -s)"; print_usage; exit 1 ;;
esac

# Use custom path or default path
CLANGFMT_BIN=${CLANGFMT_BIN:-$CLANGFMT_DEFAULT}

# Check if the binary exists and is executable
if [ ! -x "$CLANGFMT_BIN" ]; then
    echo "ERROR: $CLANGFMT_BIN not found or not executable"
    print_usage
    exit 1
fi

# Extract clang-format version
CLANGFMT_VERSION=$($CLANGFMT_BIN --version | grep -o -E "([0-9]{1,}\.)+[0-9]{1,}")

if [ "$CLANGFMT_VERSION" != "$OUR_CLANGFMT_VERSION" ]; then
    echo "ERROR: $CLANGFMT_BIN has version $CLANGFMT_VERSION, but version $OUR_CLANGFMT_VERSION is required"
    echo "       (Different versions of clang-format may produce slightly different formatting.)"
    exit 1
fi

# Check for multiple .clang-format files
CLANG_FORMAT_FILES=$(find . -name ".clang-format" | wc -l)
if [ "$CLANG_FORMAT_FILES" -gt 1 ]; then
    echo "WARNING: Multiple .clang-format files found in the project directory:"
    find . -name ".clang-format"
    echo "This may cause conflicts. Consider keeping only one .clang-format file in the project root."
fi

# Run clang-format on all relevant files
find . -regex ".*\.\(cpp\|cc\|cxx\|h\|hpp\)" ! -path "./libs/*" ! -path "./extern/*" ! -path "./d3xp/gamesys/SysCvar.cpp" ! -path "./d3xp/gamesys/Callbacks.cpp" ! -path "./sys/win32/win_cpu.cpp" ! -path "./sys/win32/win_main.cpp" -print0 | xargs -0 -P 16 "$CLANGFMT_BIN" -i

# Post-process files to align method names right (requires Python)
if command -v python >/dev/null 2>&1; then
    find . -regex ".*\.\(h\|hpp\)" ! -path "./libs/*" ! -path "./extern/*" ! -path "./d3xp/gamesys/SysCvar.cpp" ! -path "./d3xp/gamesys/Callbacks.cpp" ! -path "./sys/win32/win_cpu.cpp" ! -path "./sys/win32/win_main.cpp" -print0 | xargs -0 -I {} python align_methods.py {}
    echo "Right-alignment post-processing completed!"
else
    echo "WARNING: Python3 not found, skipping right-alignment post-processing."
fi

echo "Formatting completed!"