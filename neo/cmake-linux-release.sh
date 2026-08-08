rm -f idlib/precompiled.h.gch
rm -f tools/compilers/precompiled.h.gch
cd ..
mkdir -p build
cd build
rm -rf *
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DONATIVE=ON -DFFMPEG=OFF -DBINKDEC=ON ../neo
