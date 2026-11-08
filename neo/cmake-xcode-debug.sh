cd ..
rm -rf xcode-debug
mkdir xcode-debug
cd xcode-debug

# Define the minimum OSX deployment target for the installed sdk and machine architecture
SDK_PATH=$(xcrun --show-sdk-path 2>/dev/null)
MIN_MACOS_TARGET=$(plutil -extract SupportedTargets.macosx.MinimumDeploymentTarget raw "$SDK_PATH/SDKSettings.plist")
MIN_MACOS_SORTED=$(printf '%s\n%s' "11.0" "$MIN_MACOS_TARGET" | sort -V | head -n1)
if [ "$MIN_MACOS_SORTED" = "11.0" ]; then
	# use minimum deployment target specified by sdk if version >= macOS 11.0
	# note: MoltenVK 1.4.2 requires a minimum runtime version of macOS 12.0
	#       KosmicKrisp requires a minimum runtime version of macOS 26.0
	OSX_TARGET=$MIN_MACOS_TARGET
elif [ "$(uname -m)" = "arm64" ]; then
	# macOS 11.0 is the minimum runtime for arm64 targets and MoltenVK 1.4.1
	OSX_TARGET="11.0"
else
	# macOS 10.15 is the minimum runtime for x86_64 targets and MoltenVK <= 1.4.0
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

# note 1: remove or set -DCMAKE_SUPPRESS_REGENERATION=OFF to enable ZERO_CHECK target which checks for CMakeLists.txt changes and re-runs CMake before builds
#         however, if ZERO_CHECK is enabled **must** add Vulkan's VK_ADD_LAYER_PATH, VK_ICD_FILENAMES, and VK_DRIVER_FILES locations to Xcode Custom Paths
#         (under Settings/Locations) otherwise runtime failures may occur
# note 2: policy CMAKE_POLICY_DEFAULT_CMP0142=NEW suppresses non-existant per-config suffixes on Xcode library search paths, works for cmake version 3.25 and later
# note 3: set -DCMAKE_OSX_DEPLOYMENT_TARGET=<version> to match supported runtime targets, needed for CMake 4.0+ since CMAKE_OSX_SYSROOT is no longer set by default
# note 4: to run using KosmicKrisp vs. MoltenVK Vulkan driver define environment variable VK_DRIVER_FILES="$VULKAN_SDK/share/vulkan/icd.d/libkosmickrisp_icd.json"
cmake -G Xcode -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_SYSROOT=macosx -DCMAKE_OSX_DEPLOYMENT_TARGET=$OSX_TARGET -DFFMPEG=ON -DBINKDEC=OFF -DCMAKE_XCODE_GENERATE_SCHEME=ON -DCMAKE_XCODE_SCHEME_ENABLE_GPU_API_VALIDATION=OFF -DCMAKE_SUPPRESS_REGENERATION=ON -DOPENAL_LIBRARY=$OPENAL_PREFIX/lib/libopenal.dylib -DOPENAL_INCLUDE_DIR=$OPENAL_PREFIX/include ../neo -DCMAKE_POLICY_DEFAULT_CMP0142=NEW -Wno-dev
