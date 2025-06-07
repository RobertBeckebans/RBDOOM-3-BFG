astyle.exe -v --formatted --options=astyle-header.ini --exclude="libs" --exclude="extern" --recursive *.h
astyle.exe -v --formatted --options=astyle-cpp.ini --exclude="libs" --exclude="extern" --exclude="d3xp/gamesys/SysCvar.cpp" --exclude="d3xp/gamesys/Callbacks.cpp" --exclude="sys/win32/win_cpu.cpp" --recursive *.cpp

astyle.exe -v --formatted --options=astyle-header.ini --recursive ../doomclassic/*.h
astyle.exe -v --formatted --options=astyle-cpp.ini --recursive ../doomclassic/*.cpp

astyle.exe -v -Q --options=astyle-cpp.ini --recursive shaders/*.hlsl

pause
