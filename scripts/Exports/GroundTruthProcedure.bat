echo Sample Count of... 100!
"Haboobo.exe" --it=100 --sw=0 --of=1 --eaf=1 --sg=0 --w=1024 --h=1024 --dr=0 --o="GroundTruth.dds"
"DXT/texconv.exe" "GroundTruth.dds" -ft png -srgbo -y -wiclossless

echo Sample Count of... 24!
"Haboobo.exe" --it=24 --sw=0 --of=1 --eaf=1 --sg=0 --w=1024 --h=1024 --dr=0 --o="LowSample.dds"
"DXT/texconv.exe" "LowSample.dds" -ft png -srgbo -y -wiclossless

echo Sample Count of... 4?!
"Haboobo.exe" --it=4 --sw=0 --of=1 --eaf=1 --sg=0 --w=1024 --h=1024 --dr=0 --o="AbsurdlyLowSample.dds"
"DXT/texconv.exe" "AbsurdlyLowSample.dds" -ft png -srgbo -y -wiclossless

cmd /k