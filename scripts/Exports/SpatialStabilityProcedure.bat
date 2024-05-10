set ProgramFlags=--os=.01745329252 --of=1 --sw=0 --w=1024 --h=1024 --dr=0 --so=1 --eaf=1 --sg=0 --o="Output.dds"
set Output=SpatialStandard.csv
set /A OrbitDiscreteProgress=0
set /A OrbitProgressCount=360

:go_again
echo This is position %OrbitDiscreteProgress%
Haboobo.exe %ProgramFlags% --opi=%OrbitDiscreteProgress%

"DXT/texconv.exe" "Output.dds" -ft png -y -wiclossless

echo Lets denoise
python PyDenoiseToolApp.py Output.png 5 NoNoise.png

echo How noisy was it...?
python PyCompareToolApp.py Output.png NoNoise.png > Result.csv

echo Append to csv
python AppendCSV.py "Result.csv" %Output% "Progress" %OrbitDiscreteProgress%

set /A "OrbitDiscreteProgress = OrbitDiscreteProgress + 1"
if %OrbitProgressCount% GTR %OrbitDiscreteProgress% goto go_again

echo Notifying completion...
echo 
timeout 1
echo 
timeout 1
echo 
timeout 1
cmd /k