set MinWaitTime=5
set MaxWaitTime=6
set /A Count=5
set /A Recursions=0

:profile

START /min Profiler/capture.exe -a 127.0.0.1 -s %MinWaitTime% -o "Result.tracy" -f
START /b Haboobo.exe --ap
timeout %MaxWaitTime%
taskkill /f /IM Haboobo.exe

:pending_export
if not exist "Result.tracy" (
goto :pending_export
)

CALL "Profiler/csvexport.exe" "Result.tracy" >Result.csv
del "Result.tracy"

python "AppendCSV.py" "Result.csv" "test.txt" "ProfileNum" %Recursions%

set /A "Recursions = Recursions + 1"
if %Count% GTR %Recursions% goto profile

