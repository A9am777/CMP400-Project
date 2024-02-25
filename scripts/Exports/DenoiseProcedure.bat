echo Bad sample count of... 4?!
"Haboobo.exe" --it=4 --sw=0 --of=1 --eaf=1 --sg=0 --w=1024 --h=1024 --dr=0 --o="AbsurdlyLowSample.dds"
"DXT/texconv.exe" "AbsurdlyLowSample.dds" -ft png -srgbo -y -wiclossless

echo Lets denoise
python PyDenoiseToolApp.py AbsurdlyLowSample.png 5 NoNoise.png

echo How noisy was it...?
python PyCompareToolApp.py AbsurdlyLowSample.png NoNoise.png

cmd /k