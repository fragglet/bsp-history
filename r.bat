@echo off
setlocal
set time=%@TIME[%_time]
bsp %1
echo BSP took %@eval[%@TIME[%_time]-%time] seconds.
