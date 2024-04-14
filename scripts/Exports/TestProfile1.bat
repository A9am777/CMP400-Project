START /min Profiler/capture.exe -a 127.0.0.1 -s 5 -o "Result.tracy" -f
START /b Haboobo.exe --ap
timeout 6
taskkill /f /IM Haboobo.exe

:pending_export
if not exist "Result.tracy" (
goto :pending_export
)

CALL "Profiler/csvexport.exe" "Result.tracy" >Result.csv
cmd /k