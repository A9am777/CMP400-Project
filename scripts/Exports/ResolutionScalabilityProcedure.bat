set ProgramFlags=--of=1 --sw=0 --dr=0 --eaf=0 --sg=0 --ap
set MinWaitTime=5
set MaxWaitTime=6
set Output=Resolution.csv
set /A ResolutionProgress=1
set /A ResolutionMax=4

:go_again
echo This is resolution %ResolutionProgress%x%ResolutionProgress%

START /min Profiler/capture.exe -a 127.0.0.1 -s %MinWaitTime% -o "Result.tracy" -f
START /b Haboobo.exe %ProgramFlags% --w=%ResolutionProgress% --h=%ResolutionProgress%
timeout %MaxWaitTime%
taskkill /f /IM Haboobo.exe

:pending_export
if not exist "Result.tracy" (
goto :pending_export
)

CALL "Profiler/csvexport.exe" "Result.tracy" >Result.csv
del "Result.tracy"

echo Append to csv
python "AppendCSV.py" "Result.csv" %Output% "Resolution" %ResolutionProgress%

set /A "ResolutionProgress = ResolutionProgress + 1"
if %ResolutionMax% GEQ %ResolutionProgress% goto go_again

echo Notifying completion...
echo 
timeout 1
echo 
timeout 1
echo 
timeout 1
cmd /k