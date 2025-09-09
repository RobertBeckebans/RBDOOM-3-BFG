cd ..
rm -rf build
mkdir build
cd build

# Define the minimum OSX deployment target for machine architecture
if [ "$(uname -m)" = "arm64" ]; then
	OSX_TARGET="11.0"
else
	OSX_TARGET="10.15"
fi

# asemarafa/SRS - Determine the Homebrew path prefix for openal-soft
if [ -z "$OPENAL_PREFIX" ]; then
  OPENAL_PREFIX=$(brew --prefix openal-soft 2>/dev/null)
  if [ -z "$OPENAL_PREFIX" ]; then
  	echo "Error: openal-soft is not installed via Homebrew."
  	echo "Either install it using 'brew install openal-soft' or define the path prefix via OPENAL_PREFIX."
  	exit 1
  fi
fi

# note 1: set -DCMAKE_OSX_DEPLOYMENT_TARGET=<version> to match supported runtime targets, needed for CMake 4.0+ since CMAKE_OSX_SYSROOT is no longer set by default
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS_RELEASE="-DNDEBUG" -DCMAKE_OSX_SYSROOT=macosx -DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_TARGET -DFFMPEG=OFF -DBINKDEC=ON -DUSE_MoltenVK=ON -DOPENAL_LIBRARY=$OPENAL_PREFIX/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=$OPENAL_PREFIX/include ../neo -Wno-dev
