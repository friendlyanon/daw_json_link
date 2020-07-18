echo "Making build directory"
md build
cd build

REM ##############################
REM Setting VCVars
@call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" amd64

REM ##############################
REM Running cmake
cmake -DCMAKE_BUILD_TYPE=%1 -GNinja ..
