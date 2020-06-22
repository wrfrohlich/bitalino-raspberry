//----------------------------------------------------------------------------------------------------------
//
//			Name: Eng. William da Rosa Frohlich
//
//			Project: Library for Application of BITalino
//
//			Date: 2020.06.13
//
//----------------------------------------------------------------------------------------------------------
// Libraries

#ifndef _BITALINOHEADER_  // _BITALINOHEADER_
#define _BITALINOHEADER_

#include <string>
#include <vector>

//------------------------------------------------------------------------------------
// Windows - 32 bits or 64 bits

#ifdef _WIN32

#include <winsock2.h>

#endif

//------------------------------------------------------------------------------------
// The BITalino device class

class BITalino
{
public:

	// Type Definitions
	
	typedef std::vector<bool>  Vbool;				// Vector of bool
	typedef std::vector<int>   Vint;				// Vector of int
	
	// Information about Bluetooth device -> BITalino::find().
	struct DevInfo
	{
		std::string macAddr;	// MAC address of a Bluetooth device
		std::string name;		// Name of a Bluetooth device
	};
	typedef std::vector<DevInfo> VDevInfo; 			// Vector of DevInfo
	
	// Frame returned -> BITalino::read()
	struct Frame
	{
		// Frame sequence (0~15), incremented by 1 on each consecutive frame, can be used to detect if frames were dropped while transmitting data
		char  seq;

		// Array of digital ports states (false for low level or true for high level).
		bool  digital[4]; 
		
		// Array of analog inputs (0~1023 on 0, 1, 2 and 3 channel and 0~63 on 4 and 5 channel)
		short analog[6];
	};
	typedef std::vector<Frame> VFrame;				// Vector of Frame
	
	// Current device state returned by BITalino::state()
	struct State
	{
		// Array of analog inputs (0...1023)
		int   analog[6];
		
		// Array of digital ports states (false for low level or true for high level)
		bool  digital[4];
	};
	
	// Exception class from BITalino methods
	class Exception
	{
		public:
		
		// %Exception code enumeration.
		enum Code
		{
			INVALID_ADDRESS = 1,       // The specified address is invalid
			BT_ADAPTER_NOT_FOUND,      // No Bluetooth adapter was found
			DEVICE_NOT_FOUND,          // The device could not be found
			CONTACTING_DEVICE,         // The computer lost communication with the device
			PORT_COULD_NOT_BE_OPENED,  // The communication port does not exist or it is already being used
			PORT_INITIALIZATION,       // The communication port could not be initialized
			DEVICE_NOT_IDLE,           // The device is not idle
			DEVICE_NOT_IN_ACQUISITION, // The device is not in acquisition mode
			INVALID_PARAMETER,         // Invalid parameter
			NOT_SUPPORTED,             // Operation not supported by the device 
		} code;  // Exception code.
		
		Exception(Code c) : code(c) {}      // Exception constructor.
		const char* getDescription(void);   // Returns an exception description string
	};

//------------------------------------------------------------------------------------
// Static methods

	// Searches for Bluetooth devices in range.
	// \return a list of found devices
	// \exception Exception (Exception::PORT_INITIALIZATION)
	static VDevInfo find(void);
	
// Instance methods

	// Connects to a %BITalino device
	// The device Bluetooth MAC address ("xx:xx:xx:xx:xx:xx")
	BITalino(const char *address);
	
	// Disconnects the %BITalino device
	~BITalino();
	
	// Returns the device firmware version
	std::string version(void);
	
	// Starts a signal acquisition from the device
	// samplingRate -> Sampling rate in Hz - Accepted values are 1, 10, 100 or 1000 Hz
	// channels -> Set of channels to acquire - Accepted channels are 0...5 for inputs A1...A6
	void start(int samplingRate = 1000, const Vint &channels = Vint(), bool simulated = false);
	
	// Stops a signal acquisition
	void stop(void);
	
	//Reads acquisition frames from the device
	int read(VFrame &frames);
	
	// Assigns the digital outputs states
	void trigger(const Vbool &digitalOutput = Vbool());
	
	// Assigns the analog (PWM) output value
	void pwm(int pwmOutput = 100);
	
	// Returns current device state
	State state(void);
	
private:
	void send(char cmd);
	int  recv(void *data, int nbyttoread);
	void close(void);
	
	char nChannels;
	bool isBitalino2;

//------------------------------------------------------------------------------------
// Windows

#ifdef _WIN32
	SOCKET fd;
	timeval readtimeout;
	HANDLE hCom;

//------------------------------------------------------------------------------------
// Linux or MAC OS

#else

	int fd;
	bool isTTY;

#endif

//------------------------------------------------------------------------------------

};

#endif // _BITALINOHEADER_
//------------------------------------------------------------------------------------