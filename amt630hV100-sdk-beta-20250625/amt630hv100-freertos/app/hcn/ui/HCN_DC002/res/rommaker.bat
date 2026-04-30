:: XCOPY 命令的一些常用参数包括：
::- /s：包含子目录中的文件。
::- /e：包含空的子目录和目录树结构。
::- /i：如果目标是一个目录或一组目录，则认为源是一个目录。
::- /y：在目标文件已经存在的情况下，无需提示确认。
::- /d：仅在源文件比目标文件新或在源文件存在但目标文件缺失的情况下才复制。
::- /c：遇到错误时继续复制。
:: ============================
::copy命令的一些常用参数包括：
::- /Y：在目标文件已经存在的情况下，自动覆盖目标文件而不提示确认。
::- /-Y：在目标文件已经存在的情况下，提示确认是否覆盖目标文件。
::- /V：校验复制后的文件是否与源文件完全一致。
::- /A：复制时保留文件属性。
::- /B：以二进制模式复制文件（用于复制非文本文件）。
::
::
::设置目录参数
:: ===================
::
::SET romDIR=E:\HCN_WorkSpace\AMT630HV100\amt630hV100-sdk-beta-20250625\amt630hv100-freertos\app\hcn\ui\HCN_DC002\res
::
::制作步骤
:: ===================
::制作 amt630hv100.bin
::制作 bootanim.bin
::制作 rom.bin
:: ===================
:: 1-制作 rom.bin
:: ===================
::
::%ROM_RES_DIR%\RomMaker

rd /s/q assets\default\inc
rd /s/q assets\night\inc
::rd /s/q assets\default\inc
::rd /s/q assets\night\inc
 
::start /wait RomMaker.exe
::start rommaker.exe

rommaker.exe

pause