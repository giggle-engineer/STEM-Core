@echo off
tcc gen1802.c >>..\err.lst
gen1802 >_temp.h
sort _temp.h > _case.h

del _temp.h  >>err.lst
del gen1802.exe >>err.lst
del gen1802.obj >>err.lst
del test.obj >>err.lst
del test.exe >>err.lst
del 1802.obj >>err.lst

tcc -G -ml -DMSDOS -estemt.exe stem.c stfn.c 1802.c ext_dos.c >>..\err.lst
tcc -G -ml -DMSDOS -DGFX_VGA -estem.exe  stem.c stfn.c 1802.c ext_dos.c >>..\err.lst
del stemsrc.zip  >>err.lst

pkzip -ex stemsrc.zip make.bat *.c *.h *.ini *.def *.rc *.dsw *.ide *.obr >>..\err.lst
pkzip -d  stemsrc.zip _case.h  >>..\err.lst
copy stemsrc.zip ..\html  >>..\err.lst

del *.bak
del *.~??
