# Intro

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
