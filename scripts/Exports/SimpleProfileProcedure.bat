set ProgramFlags=--of=1 --sw=0 --w=1024 --h=1024 --dr=0 --sg=0
set MinWaitTime=120
set MaxWaitTime=125

echo Determine latency
START /min Profiler/capture.exe -a 127.0.0.1 -s %MinWaitTime% -o "Result.tracy" -f
timeout 1
START /b Haboobo.exe %ProgramFlags%

timeout %MaxWaitTime%
taskkill /f /IM Haboobo.exe

:pending_export
if not exist "Result.tracy" (
goto :pending_export
)

CALL "Profiler/csvgpuexport.exe" --unwrap "Result.tracy" >Result.csv
del "Result.tracy"

echo Notifying completion...
echo 
timeout 1
echo 
timeout 1
echo 
timeout 1
cmd /k