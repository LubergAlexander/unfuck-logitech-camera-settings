#include <CoreFoundation/CoreFoundation.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <spawn.h>
#include <mach/mach.h>
#include <signal.h>

void run_cmd_exec_exit(char *cmd, char *argv[])
{
	pid_t p = fork();

	int status;
	if (p == 0)
	{
		execv(cmd, argv);
		if (errno == ENOENT)
		{
			_exit(-1);
		}

		_exit(-2);
	}

	wait(&status);
}


void run_cmd(char *cmd, char *argv[])
{
	//char *shell[] = {"sh", "-c"};

	//int lenShell = strlen(*shell);
	//int lenArgv = strlen(*argv);

	//int sizeShell = sizeof(char *) * lenShell;
	//int sizeArgv = sizeof(char *) * lenArgv;

	//char * *total = malloc(sizeShell + sizeArgv);

	//memcpy(total, shell, sizeShell);
	//memcpy(&total[lenShell], argv, sizeArgv);

	pid_t pid;
	int status = posix_spawn(&pid, cmd, NULL, NULL, argv, NULL);

	wait(&status);
}


// globals
char *command_to_run;
char * *command_args = NULL;
static IONotificationPortRef gNotifyPort;
static io_iterator_t gRawAddedIter;
static io_iterator_t gRawRemovedIter;
IOReturn ConfigureDevice(IOUSBDeviceInterface * *dev)
{
	UInt8 numConfig;

	IOReturn kr;

	IOUSBConfigurationDescriptorPtr configDesc;

	//Get the number of configurations. The sample code always chooses

	//the first configuration (at index 0) but your code may need a

	//different one

	kr = (*dev)->GetNumberOfConfigurations(dev, &numConfig);

	if (!numConfig)
	{
		return -1;
	}

	//Get the configuration descriptor for index 0

	kr = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &configDesc);

	if (kr)
	{
		printf("Couldn’t get configuration descriptor for index %d (err = %08x)\n", 0, kr);

		return -1;
	}

	//Set the device’s configuration. The configuration value is found in

	//the bConfigurationValue field of the configuration descriptor

	kr = (*dev)->SetConfiguration(dev, configDesc->bConfigurationValue);

	if (kr)
	{
		printf("Couldn’t set configuration to value %d (err = %08x)\n", 0, kr);
		return -1;
	}

	return kIOReturnSuccess;
}


void RawDeviceAdded(void *refCon, io_iterator_t iterator)
{
	kern_return_t kr;

	io_service_t usbDevice;

	IOCFPlugInInterface * *plugInInterface = NULL;

	IOUSBDeviceInterface * *dev = NULL;

	HRESULT result;

	SInt32 score;

	UInt16 vendor;

	UInt16 product;

	UInt16 release;

	while ( (usbDevice = IOIteratorNext(iterator) ) )
	{
		//Create an intermediate plug-in

		kr = IOCreatePlugInInterfaceForService(usbDevice,

											   kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,

											   &plugInInterface, &score);

		//Don’t need the device object after intermediate plug-in is created

		kr = IOObjectRelease(usbDevice);

		if ( (kIOReturnSuccess != kr) || !plugInInterface )
		{
			printf("Unable to create a plug-in (%08x)\n", kr);

			continue;
		}

		//Now create the device interface

		result = (*plugInInterface)->QueryInterface(plugInInterface,

													CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),

													(LPVOID *)&dev);

		//Don’t need the intermediate plug-in after device interface

		//is created

		(*plugInInterface)->Release(plugInInterface);

		if (result || !dev)
		{
			printf("Couldn’t create a device interface (%08x)\n",

				   (int)result);

			continue;
		}

		//Check these values for confirmation

		kr = (*dev)->GetDeviceVendor(dev, &vendor);

		kr = (*dev)->GetDeviceProduct(dev, &product);

		kr = (*dev)->GetDeviceReleaseNumber(dev, &release);

		//Open the device to change its state

		kr = (*dev)->USBDeviceOpen(dev);

		if (kr != kIOReturnSuccess)
		{
			printf("Unable to open device: %08x\n", kr);

			(void)(*dev)->Release(dev);

			continue;
		}

		kr = (*dev)->USBDeviceClose(dev);
		kr = (*dev)->Release(dev);

		run_cmd(command_to_run, command_args);
	}
}


void RawDeviceRemoved(void *refCon, io_iterator_t iterator)
{
	kern_return_t kr;
	io_service_t object;

	while ( (object = IOIteratorNext(iterator) ) )
	{
		kr = IOObjectRelease(object);

		if (kr != kIOReturnSuccess)
		{
			printf("Couldn’t release raw device object: %08x\n", kr);
			continue;
		}
	}
}


void SignalHandler(int sigraised)
{
	printf("\nInterrupted\n");

	// Clean up here
	IONotificationPortDestroy(gNotifyPort);

	if (gRawAddedIter)
	{
		IOObjectRelease(gRawAddedIter);
		gRawAddedIter = 0;
	}

	if (gRawRemovedIter)
	{
		IOObjectRelease(gRawRemovedIter);
		gRawRemovedIter = 0;
	}

	_exit(0);
}


int main(int argc, char *argv[])
{
	mach_port_t masterPort;
	CFMutableDictionaryRef matchingDict;
	CFRunLoopSourceRef runLoopSource;
	kern_return_t kr;
	long usbVendor;
	long usbProduct;

	sig_t oldHandler;

	// pick up command line arguments
	if (argc < 3)
	{
		return -1;
	}

	usbVendor = atoi(argv[1]);
	usbProduct = atoi(argv[2]);
	command_to_run = argv[3];
	command_args = argv + 3;

	// Set up a signal handler so we can clean up when we're interrupted from the command line
	// Otherwise we stay in our run loop forever.
	oldHandler = signal(SIGINT, SignalHandler);
	if (oldHandler == SIG_ERR)
	{
		printf("Could not establish new signal handler");
	}

	// first create a master_port for my task
	kr = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (kr || !masterPort)
	{
		printf("ERR: Couldn't create a master IOKit Port(%08x)\n", kr);
		return -1;
	}

	printf("Looking for devices matching vendor ID=%ld and product ID=%ld\n", usbVendor, usbProduct);

	// Set up the matching criteria for the devices we're interested in
	matchingDict = IOServiceMatching(kIOUSBDeviceClassName);    // Interested in instances of class IOUSBDevice and its subclasses
	if (!matchingDict)
	{
		printf("Can't create a USB matching dictionary\n");
		mach_port_deallocate(mach_task_self(), masterPort);
		return -1;
	}

	// Add our vendor and product IDs to the matching criteria
	CFDictionarySetValue(
		matchingDict,
		CFSTR(kUSBVendorID),
		CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbVendor) );

	CFDictionarySetValue(
		matchingDict,
		CFSTR(kUSBProductID),
		CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &usbProduct) );

	// Create a notification port and add its run loop event source to our run loop
	// This is how async notifications get set up.
	gNotifyPort = IONotificationPortCreate(masterPort);
	runLoopSource = IONotificationPortGetRunLoopSource(gNotifyPort);

	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

	// Retain additional references because we use this same dictionary with four calls to
	// IOServiceAddMatchingNotification, each of which consumes one reference.
	matchingDict = (CFMutableDictionaryRef)CFRetain( matchingDict );
	matchingDict = (CFMutableDictionaryRef)CFRetain( matchingDict );
	matchingDict = (CFMutableDictionaryRef)CFRetain( matchingDict );

	// Now set up two notifications, one to be called when a raw device is first matched by I/O Kit, and the other to be
	// called when the device is terminated.
	kr = IOServiceAddMatchingNotification(  gNotifyPort,
											kIOFirstMatchNotification,
											matchingDict,
											RawDeviceAdded,
											NULL,
											&gRawAddedIter );

	RawDeviceAdded(NULL, gRawAddedIter);    // Iterate once to get already-present devices and
	// arm the notification

	kr = IOServiceAddMatchingNotification(  gNotifyPort,
											kIOTerminatedNotification,
											matchingDict,
											RawDeviceRemoved,
											NULL,
											&gRawRemovedIter );

	RawDeviceRemoved(NULL, gRawRemovedIter);    // Iterate once to arm the notification

	// Now done with the master_port
	mach_port_deallocate(mach_task_self(), masterPort);
	masterPort = 0;

	// Start the run loop. Now we'll receive notifications.
	CFRunLoopRun();

	// We should never get here
	return 0;
}
