%1\sqlite3.exe %2 < SQLite.sql
mkdir %1\package\plugins\BFSGSimComFiles
copy /y %1\package.ini %1\package
copy /y  %1\BFSGSimCom.dll %1\package\plugins
copy /y %2 %1\package\plugins\BFSGSimComFiles 
del %1\%3.ts3_plugin
"C:\Program Files\7-Zip\7z.exe" a -tzip -aoa -mm=Deflate %1%3.ts3_plugin %1\package\*
rmdir /s /q %1\package