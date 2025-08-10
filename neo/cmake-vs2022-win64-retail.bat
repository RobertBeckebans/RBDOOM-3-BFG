cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 17" -A x64 -DFFMPEG=ON -DBINKDEC=OFF -DRETAIL=ON -DDXC_CUSTOM_PATH=F:/Games/RBDOOM-3-BFG-git/tools/dxc071425/bin/x64 ../neo
pause