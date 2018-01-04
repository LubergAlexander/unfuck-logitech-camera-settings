const UVCControl = require('uvc-control');

// Use list-devices.js to find VID/PID for the camera. https://github.com/makenai/node-uvc-control
VID=0x46d; // 1133
PID=0x85e; // 2142

const camera = new UVCControl(VID, PID);

camera.set('autoFocus', 0);
camera.set('autoWhiteBalance', 0);
camera.set('absoluteFocus', 0);

camera.set('brightness', 130);
camera.set('sharpness', 130);
camera.set('contrast', 130);
camera.set('saturation', 130);

camera.set('whiteBalanceTemperature', 3654);

