#import <Foundation/Foundation.h>
#include <xpc/xpc.h>
#include <unistd.h>
#include "uvc/UVCCameraControl.h"

#define VID 0x46d
#define PID 0x85e

int main(int argc, const char *argv[])
{
	@autoreleasepool {
		UVCCameraControl *cameraControl = [[UVCCameraControl alloc] initWithVendorID:VID productID:PID interfaceNum:0x00];

		[cameraControl setAutoFocus:NO];
		[cameraControl setAutoWhiteBalance:NO];

		[cameraControl setAbsoluteFocus:0];
		[cameraControl setBrightness:0.5];
		[cameraControl setSharpness:0.5];
		[cameraControl setContrast:0.5];
		[cameraControl setSaturation:0.5];
		[cameraControl setWhiteBalance:0.4];
		[cameraControl setZoom:0.3];

		[cameraControl release];
	}
	return 0;
}
