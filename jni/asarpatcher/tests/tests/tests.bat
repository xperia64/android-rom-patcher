del fail
if not exist test.exe g++ test.cpp ../libstr.cpp -otest.exe -s
if exist test.exe FOR %%a IN (*.asm) DO @test.exe "%%a"
del err.log
del a.azm