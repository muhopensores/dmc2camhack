mkdir build64
cd build64
REM CLANG LINE
REM cmake .. -G "Visual Studio 15 2017 Win64" -T LLVM_v141 -DDEVELOPER_MODE=OFF
REM ICL LINE
REM cmake .. -G "Visual Studio 15 2017 Win64" -T "Intel C++ Compiler 19.0" -DDEVELOPER_MODE=OFF
cmake .. -G "Visual Studio 15 2017 Win64" -DDEVELOPER_MODE=OFF
cmake --build . --config Debug
cd ..
pause