cd ..
rm -rf xcode-release
mkdir xcode-release
cd xcode-release

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

# note 1: policy CMAKE_POLICY_DEFAULT_CMP0142=NEW suppresses non-existant per-config suffixes on Xcode library search paths, works for cmake version 3.25 and later
# note 2: env variable OS_ACTIVITY_MODE=disable deactivates OS logging to Xcode console - specifically this suppresses Apple AUHAL status message spam from OpenAL
# note 3: set -DCMAKE_OSX_DEPLOYMENT_TARGET=<version> to match supported runtime targets, needed for CMake 4.0+ since CMAKE_OSX_SYSROOT is no longer set by default
cmake -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_SYSROOT=macosx -DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_TARGET -DCMAKE_CONFIGURATION_TYPES="Release;MinSizeRel;RelWithDebInfo" -DMACOSX_BUNDLE=ON -DFFMPEG=OFF -DBINKDEC=ON -DUSE_MoltenVK=ON -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_XCODE_SCHEME_ENVIRONMENT="OS_ACTIVITY_MODE=disable" -DCMAKE_XCODE_SCHEME_ENABLE_GPU_API_VALIDATION=OFF -DOPENAL_LIBRARY=$OPENAL_PREFIX/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=$OPENAL_PREFIX/include ../neo -DCMAKE_POLICY_DEFAULT_CMP0142=NEW -Wno-dev
