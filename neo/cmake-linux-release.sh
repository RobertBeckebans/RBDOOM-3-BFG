rm -f idlib/precompiled.h.gch
rm -f tools/compilers/precompiled.h.gch
cd ..
rm -rf build
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DONATIVE=ON -DFFMPEG=OFF -DBINKDEC=ON ../neo
