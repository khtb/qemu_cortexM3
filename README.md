# ARM Cortex-M3 (QEMU)

Small bare-metal C project for Cortex-M3.  based on ARM lm3s6965evb 

  
steps :  
- [x] integrate RTOS  
- [x] set up gdb debug in vscode  
- [x] test uart with Qemu terminal 
- [ ] Ethernet driver integration
- [ ] create py file to test traffic between host <-> qemu 



### Build and run:
---
to start QEMU 
```
make run
```

### Environmet 
---
.vscode settings to launch debug and start QEMU
debug uses Cortex-Debug vscode extension.

make sure that `arm-none-eabi-gdb` is configured in cortex-debug settings.

example 
``` json
  "cortex-debug.armToolchainPath": "/opt/homebrew/bin",
  "cortex-debug.gdbPath": "/opt/homebrew/bin/arm-none-eabi-gdb",
  "cortex-debug.armToolchainPrefix": "arm-none-eabi",
  "cortex-debug.openocdPath": "/opt/homebrew/bin/openocd"
```


