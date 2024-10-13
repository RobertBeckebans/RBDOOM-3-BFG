cd ..
del /s /q build
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -DWINDOWS10=ON ../neo
pause