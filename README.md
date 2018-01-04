# Intro


Logitech BRIO resets all settings after re-connecting the device.  It makes it virtually impossible to use with a laptop.

This daemon will listen to device connections and once camera is connected it sets the correct settings.

# Overview


- daemon - listens to RawDeviceAdded with a given VID & PID and triggers the command from argv
```sh
./daemon <VID> <PID> <command> <command_args>
```

- settings - small cli to set hardcoded camera settings via UVC
- run.sh - for delayed execution since camera takes time to initialize

# Build


`make all`

# Install


`make install`
