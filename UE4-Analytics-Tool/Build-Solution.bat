@ECHO OFF

FOR %%a IN (.) DO SET currentfolder=%%~nxa

IF NOT EXIST ./Build GOTO CREATEBUILD
:CREATEBUILD
	@ECHO New Build Folder Created ...
	md "./Build"
	
cd ./Build

conan install .. 

cmake .. -G "Visual Studio 14 Win64"

cmake --build . --config Release #ask cmake to build in Release mode

cd ..
	
pause
