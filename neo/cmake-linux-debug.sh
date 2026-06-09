rm -f idlib/precompiled.h.gch
rm -f tools/compilers/precompiled.h.gch
cd ..
mkdir -p build
cd build
rm -rf *
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DFFMPEG=ON -DBINKDEC=OFF -DCMAKE_CXX_COMPILER=g++ -DUSE_PRECOMPILED_HEADERS=OFF ../neo
