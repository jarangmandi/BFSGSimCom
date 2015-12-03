mkdir %1\package
copy %1\package.ini %1\package
mkdir %1\package\plugins
copy %1\BFSGSimCom.dll %1\package\plugins
del %1\%2.ts3_plugin
"C:\Program Files\7-Zip\7z.exe" a -tzip -aoa -mm=Deflate %1\%2.ts3_plugin %1\package\*
rmdir /s /q %1\package