@echo off 
set ccf=-MTd -nologo -fp:fast -Gm- -GR- -EHa- -Zo -Od -Oi -W4 -FC -Z7 -Fedk30_fall_2021_ -EHsc -wd 4189 
set ccl=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib 
IF NOT EXIST ..\build mkdir ..\build 
pushd ..\build 
IF [%1] == [a] cl %ccf% -c -Ycpch.h ..\code\pch.cpp
cl %ccf% -Yupch.h /Tp ..\code\unity.build -link %ccl% pch.obj
popd 
