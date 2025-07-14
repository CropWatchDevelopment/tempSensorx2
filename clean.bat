@echo off
:: Clean all .o and .d files in the current directory and subdirectories

echo Cleaning all .o and .d files...
for /r %%x in (*.o *.d) do (
    echo Deleting %%x
    del "%%x"
)

echo Done!
pause