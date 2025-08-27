@echo off
REM OrderEngine Windows Build Script (Phase 1)

echo Building OrderEngine Phase 1...

REM 创建目录
if not exist "build" mkdir "build"
if not exist "build\common" mkdir "build\common"
if not exist "build\network" mkdir "build\network"
if not exist "bin" mkdir "bin"
if not exist "lib" mkdir "lib"

REM 设置编译参数
set CXX=g++
set CXXFLAGS=-std=c++17 -Wall -Wextra -O0 -g3 -DORDER_ENGINE_DEBUG
set INCLUDES=-Iinclude -Iexternal\spdlog\include
set LDFLAGS=-pthread

echo Compiling common modules...
%CXX% %CXXFLAGS% %INCLUDES% -c src\common\logger.cpp -o build\common\logger.o
if errorlevel 1 goto error

echo Compiling network modules...
%CXX% %CXXFLAGS% %INCLUDES% -c src\network\poller.cpp -o build\network\poller.o
if errorlevel 1 goto error

%CXX% %CXXFLAGS% %INCLUDES% -c src\network\channel.cpp -o build\network\channel.o
if errorlevel 1 goto error

%CXX% %CXXFLAGS% %INCLUDES% -c src\network\reactor.cpp -o build\network\reactor.o
if errorlevel 1 goto error

%CXX% %CXXFLAGS% %INCLUDES% -c src\network\tcp_connection.cpp -o build\network\tcp_connection.o
if errorlevel 1 goto error

%CXX% %CXXFLAGS% %INCLUDES% -c src\network\tcp_server.cpp -o build\network\tcp_server.o
if errorlevel 1 goto error

echo Creating static library...
ar rcs lib\liborder_engine_core.a build\common\*.o build\network\*.o
if errorlevel 1 goto error

echo Compiling main application...
%CXX% %CXXFLAGS% %INCLUDES% -c src\main.cpp -o build\main.o
if errorlevel 1 goto error

echo Linking main application...
%CXX% %LDFLAGS% build\main.o -Llib -lorder_engine_core -o bin\order_engine.exe
if errorlevel 1 goto error

echo Build completed successfully!
echo Executable: bin\order_engine.exe
goto end

:error
echo Build failed!
exit /b 1

:end
