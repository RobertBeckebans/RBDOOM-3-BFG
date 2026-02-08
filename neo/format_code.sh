#!/bin/sh

# Desired clang-format version
REQUIRED_VERSION=18.1.8

print_usage () {
    echo "Please ensure that clang-format version $REQUIRED_VERSION is used, as other versions may produce different formatting!"
    echo "Set the CLANGFMT_BIN environment variable to specify a custom path to clang-format."
    echo "Example: CLANGFMT_BIN=/usr/bin/clang-format $0"
}

# Determine platform and set clang-format binary path
case "$(uname -s)" in
    Linux*)
        if command -v clang-format-18 >/dev/null 2>&1; then
            CLANGFMT_DEFAULT="clang-format-18"
        elif command -v clang-format >/dev/null 2>&1; then
            CLANGFMT_DEFAULT="clang-format"
        else
            CLANGFMT_DEFAULT="./clang-format"
        fi
        ;;
    MINGW*|MSYS*|CYGWIN*) CLANGFMT_DEFAULT="./clang-format.exe" ;;
    *)        echo "ERROR: Unsupported platform: $(uname -s)"; print_usage; exit 1 ;;
esac

# Use custom path or default path
CLANGFMT_BIN=${CLANGFMT_BIN:-$CLANGFMT_DEFAULT}

# Check if the binary exists and is executable
if ! command -v "$CLANGFMT_BIN" >/dev/null 2>&1 && [ ! -x "$CLANGFMT_BIN" ]; then
    echo "ERROR: $CLANGFMT_BIN not found or not executable"
    print_usage
    exit 1
fi

# Extract clang-format version, take only the first match
CLANGFMT_VERSION="$($CLANGFMT_BIN --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1)"
if [ -z "$CLANGFMT_VERSION" ]; then
    echo "ERROR: Could not extract version from $CLANGFMT_BIN --version"
    exit 1
fi

# Clean the version strings
CLANGFMT_VERSION_CLEAN="$(echo "$CLANGFMT_VERSION" | tr -d '\r\n[:space:]' | sed 's/[^0-9.]//g')"
REQUIRED_VERSION_CLEAN="$(echo "$REQUIRED_VERSION" | tr -d '\r\n[:space:]' | sed 's/[^0-9.]//g')"

# Debug output to inspect values
#echo "DEBUG: CLANGFMT_BIN: $CLANGFMT_BIN"
#echo "DEBUG: Raw CLANGFMT_VERSION: '$CLANGFMT_VERSION'"
#echo "DEBUG: Cleaned CLANGFMT_VERSION_CLEAN: '$CLANGFMT_VERSION_CLEAN'"
#echo "DEBUG: Cleaned REQUIRED_VERSION_CLEAN: '$REQUIRED_VERSION_CLEAN'"

# Compare versions
if [ "$CLANGFMT_VERSION_CLEAN" != "$REQUIRED_VERSION_CLEAN" ]; then
    echo "ERROR: $CLANGFMT_BIN has version $CLANGFMT_VERSION, but version $REQUIRED_VERSION is required"
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

# Copy different configs because -style=file: did not work
cp .clang-format-header .clang-format
find . -regex ".*\.\(h\|hpp\)" \
	! -path "./build/*" \
	! -path "./libs/*" \
	! -path "./extern/*" \
	! -path "./idlib/sys/sys_defines.h" \
	! -path "./renderer/Image_blueNoiseVC_1M.h" \
	! -path "./renderer/Image_blueNoiseVC_2.h" \
	! -path "./renderer/Image_brdfLut.h" \
	! -path "./renderer/Image_env_UAC_lobby_amb.h" \
	! -path "./renderer/Image_env_UAC_lobby_spec.h" \
	! -path "./renderer/SMAA/AreaTex.h" \
	! -path "./renderer/SMAA/SearchTex.h" \
    ! -path "./renderer/simplex.h" \
    ! -path "./d3xp/gamesys/GameTypeInfo.h" \
	-print0 | xargs -0 -P 16 "$CLANGFMT_BIN" -i --verbose

cp .clang-format-cpp .clang-format
find . -regex ".*\.\(c\|cpp\)" \
	! -path "./libs/*" \
	! -path "./extern/*" \
	! -path "./d3xp/gamesys/SysCvar.cpp" \
	! -path "./d3xp/gamesys/Callbacks.cpp" \
	! -path "./sys/win32/win_cpu.cpp" \
	! -path "./sys/win32/win_main.cpp" \
	-print0 | xargs -0 -P 16 "$CLANGFMT_BIN" -i --verbose

rm .clang-format

# Post-process files for ( void ), (void) -> () (requires Python)
# This might break some C code so outcommented this if you run into problems
if command -v python >/dev/null 2>&1; then
    python format_slimvoids.py
    echo "Void post-processing completed!"
elif command -v python3 >/dev/null 2>&1; then
    python3 format_slimvoids.py
    echo "Void post-processing completed!"
else
    echo "WARNING: Python not found, skipping right-alignment post-processing."
fi

echo "Formatting completed!"
