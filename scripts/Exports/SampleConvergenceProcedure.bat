set ProgramFlags=--of=1 --sw=0 --w=1024 --h=1024 --dr=0 --sg=0
set MinWaitTime=5
set MaxWaitTime=6
set OutputConvergence=Convergence.csv
set OutputLatency=MarchLatency.csv
set /A MarchSampleProgress=1
set /A MarchSampleMax=16

echo Capture the 'ground truth'
Haboobo.exe %ProgramFlags% --it=%MarchSampleMax% --eaf=1 --o="GroundTruth.dds"
"DXT/texconv.exe" "GroundTruth.dds" -ft png -y -wiclossless

echo Start to loop samples
:go_again
echo This is sample number %MarchSampleProgress%

echo Determine latency first
START /min Profiler/capture.exe -a 127.0.0.1 -s %MinWaitTime% -o "Result.tracy" -f
START /b Haboobo.exe --it=%MarchSampleProgress% %ProgramFlags%
timeout %MaxWaitTime%
taskkill /f /IM Haboobo.exe

:pending_export
if not exist "Result.tracy" (
goto :pending_export
)

CALL "Profiler/csvexport.exe" "Result.tracy" >Result.csv
del "Result.tracy"

echo Append to csv
python "AppendCSV.py" "Result.csv" %OutputLatency% "Samples" %MarchSampleProgress%

echo Now determine convergence
Haboobo.exe --it=%MarchSampleProgress% %ProgramFlags% --eaf=1 --o="Output.dds"
"DXT/texconv.exe" "Output.dds" -ft png -y -wiclossless

echo Compare to ground truth
python PyCompareToolApp.py Output.png GroundTruth.png > Result.csv

echo Append to csv
python AppendCSV.py "Result.csv" %OutputConvergence% "Samples" %MarchSampleProgress%

set /A "MarchSampleProgress = MarchSampleProgress + 1"
if %MarchSampleMax% GTR %MarchSampleProgress% goto go_again

echo Notifying completion...
echo 
timeout 1
echo 
timeout 1
echo 
timeout 1
cmd /k