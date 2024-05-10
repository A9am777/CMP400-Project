set ProgramFlags=--sw=0 --dr=0 --eaf=0 --sg=0 --ap
set MinWaitTime=5
set MaxWaitTime=6
set Output=ResolutionStandard.csv
set /A ResolutionProgress=16
set /A ResolutionStep=16
set /A ResolutionMax=4096

:go_again
echo This is resolution %ResolutionProgress%x%ResolutionProgress%

START /min Profiler/capture.exe -a 127.0.0.1 -s %MinWaitTime% -o "Result.tracy" -f
timeout 1
START /b Haboobo.exe %ProgramFlags% --w=%ResolutionProgress% --h=%ResolutionProgress%

timeout %MaxWaitTime%
taskkill /f /IM Haboobo.exe

:pending_export
if not exist "Result.tracy" (
goto :pending_export
)

CALL "Profiler/csvgpuexport.exe" "Result.tracy" >Result.csv
del "Result.tracy"

echo Append to csv
python "AppendCSV.py" "Result.csv" %Output% "Resolution" %ResolutionProgress%

set /A "ResolutionProgress = ResolutionProgress + ResolutionStep"
if %ResolutionMax% GEQ %ResolutionProgress% goto go_again

echo Notifying completion...
echo 
timeout 1
echo 
timeout 1
echo 
timeout 1
cmd /k