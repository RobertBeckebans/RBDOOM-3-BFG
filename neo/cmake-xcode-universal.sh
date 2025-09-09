cd ..
rm -rf xcode-universal
mkdir xcode-universal
cd xcode-universal

# SRS - Determine if libsdl2 universal variant is installed via MacPorts
SDL2_VARIANTS=$(port info --variants libsdl2 2>/dev/null)
if [[ $SDL2_VARIANTS != *universal* ]]; then
  echo "Error: libsdl2 universal variant is not installed via MacPorts."
  echo "Please install it using 'sudo port install libsdl2 +universal'"
  exit 1
fi

# SRS - Determine if openal-soft universal variant is installed via MacPorts
OPENAL_VARIANTS=$(port info --variants openal-soft 2>/dev/null)
if [[ $OPENAL_VARIANTS != *universal* ]]; then
  echo "Error: openal-soft universal variant is not installed via MacPorts."
  echo "Please install it using 'sudo port install openal-soft +universal'"
  exit 1
fi

# note 1: policy CMAKE_POLICY_DEFAULT_CMP0142=NEW suppresses non-existant per-config suffixes on Xcode library search paths, works for cmake version 3.25 and later
# note 2: universal openal-soft library and include paths assume MacPorts install locations
# note 3: env variable OS_ACTIVITY_MODE=disable deactivates OS logging to Xcode console - specifically this suppresses Apple AUHAL status message spam from OpenAL
# note 4: set -DCMAKE_OSX_DEPLOYMENT_TARGET=<version> to match supported runtime targets, needed for CMake 4.0+ since CMAKE_OSX_SYSROOT is no longer set by default
cmake -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_SYSROOT=macosx -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_CONFIGURATION_TYPES="Release;MinSizeRel;RelWithDebInfo" -DMACOSX_BUNDLE=ON -DFFMPEG=OFF -DBINKDEC=ON -DUSE_MoltenVK=ON -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_XCODE_SCHEME_ENVIRONMENT="OS_ACTIVITY_MODE=disable" -DCMAKE_XCODE_SCHEME_ENABLE_GPU_API_VALIDATION=OFF -DOPENAL_LIBRARY=/opt/local/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=/opt/local/include ../neo -DCMAKE_POLICY_DEFAULT_CMP0142=NEW -Wno-dev
