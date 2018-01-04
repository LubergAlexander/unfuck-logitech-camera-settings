BUILD_DIR = build

.PHONY: all clean main settings install

all: clean main settings

settings: settings/uvc/UVCCameraControl.h settings/uvc/UVCCameraControl.m settings/main.m
	clang -framework Foundation -framework IOKit settings/uvc/UVCCameraControl.m settings/main.m -o $(BUILD_DIR)/settings
	cp settings/run.sh build/

main: daemon/main.c
	clang -framework CoreFoundation -framework IOKit daemon/main.c -o $(BUILD_DIR)/daemon

install:
	cp daemon/logitech-cam-settings.plist ~/Library/LaunchAgents
	launchctl unload ~/Library/LaunchAgents/logitech-cam-settings.plist
	launchctl load ~/Library/LaunchAgents/logitech-cam-settings.plist

clean:
	rm -f $(BUILD_DIR)/*

