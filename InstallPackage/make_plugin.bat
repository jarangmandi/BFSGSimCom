echo %1
echo %2
echo %3
%1sqlite3.exe %1%2.db < SQLite.sql
mkdir %1package\plugins\%2_plugin
copy /y %1package.ini %1package
copy /y  %1\%2.dll %1package\plugins\%2_plugin_%3.dll
copy /y %1%2.db %1package\plugins\%2_plugin
del %1\%2_%3.ts3_plugin
"C:\Program Files\7-Zip\7z.exe" a -tzip -aoa -mm=Deflate %1%2_%3.ts3_plugin %1package\*
rmdir /s /q %1package