"Haboobo.exe" --sw=0 --of=1 --eaf=1 --sg=0 --w=1024 --h=1024 --dr=0 --o="Snapshot.dds"
"DXT/texconv.exe" "Snapshot.dds" -ft png -srgbo -y -wiclossless
cmd /k