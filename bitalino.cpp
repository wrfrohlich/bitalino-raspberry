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

// Windows - 32 bits or 64 bits
#ifdef _WIN32 

#define HASBLUETOOTH

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2bth.h>

//------------------------------------------------------------------------------------
// Linux or Mac OS

#else

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

//------------------------------------------------------------------------------------
// Linux only

#ifdef HASBLUETOOTH  // HASBLUETOOTH

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <stdlib.h>

#endif // HASBLUETOOTH

//------------------------------------------------------------------------------------

void Sleep(int millisecs)
{
   usleep(millisecs*1000);
}

#endif
//------------------------------------------------------------------------------------

#include "bitalino.h"

//------------------------------------------------------------------------------------
// CRC4 check function

static const unsigned char CRC4tab[16] = {0, 3, 6, 5, 12, 15, 10, 9, 11, 8, 13, 14, 7, 4, 1, 2};

static bool checkCRC4(const unsigned char *data, int len)
{
   unsigned char crc = 0;

   for (int i = 0; i < len-1; i++)
   {
      const unsigned char b = data[i];
      crc = CRC4tab[crc] ^ (b >> 4);
      crc = CRC4tab[crc] ^ (b & 0x0F);
   }

   // CRC for last byte
   crc = CRC4tab[crc] ^ (data[len-1] >> 4);
   crc = CRC4tab[crc];

   return (crc == (data[len-1] & 0x0F));
}

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// BITalino public methods

BITalino::VDevInfo BITalino::find(void)
{
	VDevInfo devs;
	DevInfo  devInfo;
	
//------------------------------------------------------------------------------------
// Windows

#ifdef _WIN32
	char addrStr[40];
	WSADATA m_data;
	
	if (WSAStartup(0x202, &m_data) != 0)	throw Exception(Exception::PORT_INITIALIZATION);
	
	WSAQUERYSETA querySet;
	ZeroMemory(&querySet, sizeof querySet);
	querySet.dwSize = sizeof(querySet);
	querySet.dwNameSpace = NS_BTH;
	
	HANDLE hLookup;
	DWORD flags = LUP_CONTAINERS | LUP_RETURN_ADDR | LUP_RETURN_NAME | LUP_FLUSHCACHE;
	bool tryempty = true;
	bool again;
	
	do
	{
		again = false;
		if (WSALookupServiceBeginA(&querySet, flags, &hLookup) != 0)
		{
			WSACleanup();
			throw Exception(Exception::BT_ADAPTER_NOT_FOUND);
		}
		
		while (1)
		{
			BYTE buffer[1500];
			DWORD bufferLength = sizeof(buffer);
			WSAQUERYSETA *pResults = (WSAQUERYSETA*)&buffer;
			if (WSALookupServiceNextA(hLookup, flags, &bufferLength, pResults) != 0)	break;
			if (pResults->lpszServiceInstanceName[0] == 0 && tryempty)
			{
				tryempty = false;
				again = true;
				break;
			}
			
			DWORD strSiz = sizeof addrStr;
			if (WSAAddressToStringA(pResults->lpcsaBuffer->RemoteAddr.lpSockaddr, pResults->lpcsaBuffer->RemoteAddr.iSockaddrLength, NULL, addrStr, &strSiz) == 0)
			{
				addrStr[strlen(addrStr)-1] = 0;   // remove trailing ')'
				devInfo.macAddr = addrStr+1;   // remove leading '('
				devInfo.name = pResults->lpszServiceInstanceName;
				devs.push_back(devInfo);
			}
		}
		
		WSALookupServiceEnd(hLookup);
	} 
	while (again);
	
	WSACleanup();

//------------------------------------------------------------------------------------
// Linux or Mac OS
#else

#ifdef HASBLUETOOTH
    
#define MAX_DEVS 255
	int dev_id = hci_get_route(NULL);
	int sock = hci_open_dev(dev_id);
	if (dev_id < 0 || sock < 0)
		throw Exception(Exception::PORT_INITIALIZATION);
	inquiry_info ii[MAX_DEVS];
	inquiry_info *pii = ii;
	
	int num_rsp = hci_inquiry(dev_id, 8, MAX_DEVS, NULL, &pii, IREQ_CACHE_FLUSH);
	if(num_rsp < 0)
	{
		::close(sock);
		throw Exception(Exception::PORT_INITIALIZATION);
	}
	
	for (int i = 0; i < num_rsp; i++)
	{
		char addr[19], name[248];
		ba2str(&ii[i].bdaddr, addr);
		if (hci_read_remote_name(sock, &ii[i].bdaddr, sizeof name, name, 0) >= 0)
		{
			devInfo.macAddr = addr;
			devInfo.name = name;
			devs.push_back(devInfo);
		}
	}
	
	::close(sock);
	if (pii != ii)   free(pii);
   
#else

	throw Exception(Exception::BT_ADAPTER_NOT_FOUND);
   
#endif // HASBLUETOOTH

#endif // Linux or Mac OS
//------------------------------------------------------------------------------------

    return devs;
}

//------------------------------------------------------------------------------------
BITalino::BITalino(const char *address) : nChannels(0), isBitalino2(false)
{

//------------------------------------------------------------------------------------
// Windows

#ifdef _WIN32
	if (_memicmp(address, "COM", 3) == 0)
	{
		fd = INVALID_SOCKET;
		
		char xport[40] = "\\\\.\\";   // preppend "\\.\"
		
		strcat_s(xport, 40, address);
		hCom = CreateFileA(xport, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hCom == INVALID_HANDLE_VALUE)
			throw Exception(Exception::PORT_COULD_NOT_BE_OPENED);
		DCB dcb;
		if (!GetCommState(hCom, &dcb))
		{
			close();
			throw Exception(Exception::PORT_INITIALIZATION);
		}
		dcb.BaudRate = CBR_115200;
		dcb.fBinary = TRUE;
		dcb.fParity = FALSE;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		dcb.fDtrControl = DTR_CONTROL_DISABLE;
		dcb.fDsrSensitivity = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fNull = FALSE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
		dcb.ByteSize = 8;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		if (!SetCommState(hCom, &dcb))
		{
			close();
			throw Exception(Exception::PORT_INITIALIZATION);
		}
		
		COMMTIMEOUTS ct;
		ct.ReadIntervalTimeout         = 0;
		ct.ReadTotalTimeoutConstant    = 5000; // 5 s
		ct.ReadTotalTimeoutMultiplier  = 0;
		ct.WriteTotalTimeoutConstant   = 5000; // 5 s
		ct.WriteTotalTimeoutMultiplier = 0;
		
		if (!SetCommTimeouts(hCom, &ct))
		{
			close();
			throw Exception(Exception::PORT_INITIALIZATION);
		}
	}
	else // Address is a Bluetooth MAC address
	{
		hCom = INVALID_HANDLE_VALUE;
		
		WSADATA m_data;
		if (WSAStartup(0x202, &m_data) != 0)
			throw Exception(Exception::PORT_INITIALIZATION);
		
		SOCKADDR_BTH so_bt;
		int siz = sizeof so_bt;
		
		if (WSAStringToAddressA((LPSTR)address, AF_BTH, NULL, (sockaddr*)&so_bt, &siz) != 0)
		{
			WSACleanup();
			throw Exception(Exception::INVALID_ADDRESS);
		}
		
		so_bt.port = 1;
		
		fd = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
		
		if (fd == INVALID_SOCKET)
		{
			WSACleanup();
			throw Exception(Exception::PORT_INITIALIZATION);
		}
		
		DWORD rcvbufsiz = 128*1024; // 128k
		
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*) &rcvbufsiz, sizeof rcvbufsiz);
		
		if (connect(fd, (const sockaddr*)&so_bt, sizeof so_bt) != 0)
		{
			int err = WSAGetLastError();
			close();
			
			switch(err)
			{
				case WSAENETDOWN:
					throw Exception(Exception::BT_ADAPTER_NOT_FOUND);
				
				case WSAETIMEDOUT:
					throw Exception(Exception::DEVICE_NOT_FOUND);
				
				default:
					throw Exception(Exception::PORT_COULD_NOT_BE_OPENED);
			}
		}
		
		readtimeout.tv_sec = 5;
		readtimeout.tv_usec = 0;
	}

//------------------------------------------------------------------------------------
// Linux or Mac OS

#else
	if (memcmp(address, "/dev/", 5) == 0)
	{
		fd = open(address, O_RDWR | O_NOCTTY | O_NDELAY);
		if (fd < 0)
			throw Exception(Exception::PORT_COULD_NOT_BE_OPENED);
		
		if (fcntl(fd, F_SETFL, 0) == -1)  // Remove the O_NDELAY flag
		{
			close();
			throw Exception(Exception::PORT_INITIALIZATION);
		}
		
		termios term;
		
		if (tcgetattr(fd, &term) != 0)
		{
			close();
			throw Exception(Exception::PORT_INITIALIZATION);
		}
		
		cfmakeraw(&term);
		term.c_oflag &= ~(OPOST);
		
		term.c_cc[VMIN] = 1;
		term.c_cc[VTIME] = 1;
		term.c_iflag &= ~(INPCK | PARMRK | ISTRIP | IGNCR | ICRNL | INLCR | IXON | IXOFF | IMAXBEL); // no flow control
		term.c_iflag |= (IGNPAR | IGNBRK);
		
		term.c_cflag &= ~(CRTSCTS | PARENB | CSTOPB | CSIZE); // no parity, 1 stop bit
		term.c_cflag |= (CLOCAL | CREAD | CS8);    // raw mode, 8 bits
		
		term.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOPRT | ECHOK | ECHOKE | ECHONL | ECHOCTL | ISIG | IEXTEN | TOSTOP);  // raw mode
		
		if (cfsetspeed(&term, B115200) != 0)
		{
			close();
			throw Exception(Exception::PORT_INITIALIZATION);
		}
		
		if (tcsetattr(fd, TCSANOW, &term) != 0)
		{
			close();
			throw Exception(Exception::PORT_INITIALIZATION);
		}
		
		isTTY = true;
	}
	else // address is a Bluetooth MAC address
	
#ifdef HASBLUETOOTH

	{
		sockaddr_rc so_bt;
		so_bt.rc_family = AF_BLUETOOTH;
		if (str2ba(address, &so_bt.rc_bdaddr) < 0)
			throw Exception(Exception::INVALID_ADDRESS);
		so_bt.rc_channel = 1;
		
		fd = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
		if (fd < 0)
			throw Exception(Exception::PORT_INITIALIZATION);
		
		if (connect(fd, (const sockaddr*)&so_bt, sizeof so_bt) != 0)
		{
			close();
			throw Exception(Exception::PORT_COULD_NOT_BE_OPENED);
		}
		
		isTTY = false;
	}
#else
	
	throw Exception(Exception::PORT_COULD_NOT_BE_OPENED);

#endif // HASBLUETOOTH

//------------------------------------------------------------------------------------
// Linux or Mac OS
#endif

	// Check if device is BITalino2
	const std::string ver = version();
	const std::string::size_type pos = ver.find("_v");
	if (pos != std::string::npos)
	{
		const char *xver = ver.c_str() + pos+2;
		if (atoi(xver) >= 5)  isBitalino2 = true;
		}
}

//------------------------------------------------------------------------------------

BITalino::~BITalino(void)
{
	try
	{
		if (nChannels != 0)  stop();
	}
	catch (Exception) {}
	
	close();
}

//------------------------------------------------------------------------------------

std::string BITalino::version(void)
{
	if (nChannels != 0)   throw Exception(Exception::DEVICE_NOT_IDLE);
	
	const char *header = "BITalino";
	
	const size_t headerLen = strlen(header);
	
	send(0x07);    // 0  0  0  0  0  1  1  1 - Send version string
	
	std::string str;
	
	while(1)
	{
		char chr;
		if (recv(&chr, sizeof chr) != sizeof chr)    // a timeout has occurred
			throw Exception(Exception::CONTACTING_DEVICE);
			
		const size_t len = str.size();
		if (len >= headerLen)
		{
			if (chr == '\n')  return str;
			str.push_back(chr);
		}
		else
			if (chr == header[len])
		str.push_back(chr);
		else
		{
			str.clear();   // discard all data before version header
			if (chr == header[0])   str.push_back(chr);
		}
	}
}

//------------------------------------------------------------------------------------

void BITalino::start(int samplingRate, const Vint &channels, bool simulated)
{
	if (nChannels != 0)   throw Exception(Exception::DEVICE_NOT_IDLE);
	
	unsigned char cmd;
	switch (samplingRate)
	{
		case 1:
			cmd = 0x03;
			break;
		case 10:
			cmd = 0x43;
			break;
		case 100:
			cmd = 0x83;
			break;
		case 1000:
			cmd = 0xC3;
			break;
		default:
			throw Exception(Exception::INVALID_PARAMETER);
	}
	
	char chMask;
	if (channels.empty())
	{
		chMask = 0x3F;    // all 6 analog channels
		nChannels = 6;
	}
	else
	{
		chMask = 0;
		nChannels = 0;
		for(Vint::const_iterator it = channels.begin(); it != channels.end(); it++)
		{
			int ch = *it;
			if (ch < 0 || ch > 5)   throw Exception(Exception::INVALID_PARAMETER);
			const char mask = 1 << ch;
			if (chMask & mask)   throw Exception(Exception::INVALID_PARAMETER);
			chMask |= mask;
			nChannels++;
		}
	}
	
	send(cmd);
	
	send((chMask << 2) | (simulated ? 0x02 : 0x01));
}

//------------------------------------------------------------------------------------

void BITalino::stop(void)
{
	if (nChannels == 0)   throw Exception(Exception::DEVICE_NOT_IN_ACQUISITION);
	
	send(0x00); // 0  0  0  0  0  0  0  0 - Go to idle mode
	
	nChannels = 0;
	
	version();  // to flush pending frames in input buffer
}

//------------------------------------------------------------------------------------

int BITalino::read(VFrame &frames)
{
	if (nChannels == 0)   throw Exception(Exception::DEVICE_NOT_IN_ACQUISITION);
	
	unsigned char buffer[8]; // frame maximum size is 8 bytes
	
	if (frames.empty())   frames.resize(100);

	char nBytes = nChannels + 2;
	if (nChannels >= 3 && nChannels <= 5)  nBytes++;

	for(VFrame::iterator it = frames.begin(); it != frames.end(); it++)
	{
		if (recv(buffer, nBytes) != nBytes)    return int(it - frames.begin());   // Timeout has occurred
		
		while (!checkCRC4(buffer, nBytes))
		{
			memmove(buffer, buffer+1, nBytes-1);
			if (recv(buffer+nBytes-1, 1) != 1)    return int(it - frames.begin());   // Timeout has occurred
		}
		
		Frame &f = *it;
		
		f.seq = buffer[nBytes-1] >> 4;
		for(int i = 0; i < 4; i++)
			f.digital[i] = ((buffer[nBytes-2] & (0x80 >> i)) != 0);
		f.analog[0] = (short(buffer[nBytes-2] & 0x0F) << 6) | (buffer[nBytes-3] >> 2);
		if (nChannels > 1)
			f.analog[1] = (short(buffer[nBytes-3] & 0x03) << 8) | buffer[nBytes-4];
		if (nChannels > 2)
			f.analog[2] = (short(buffer[nBytes-5]) << 2) | (buffer[nBytes-6] >> 6);
		if (nChannels > 3)
			f.analog[3] = (short(buffer[nBytes-6] & 0x3F) << 4) | (buffer[nBytes-7] >> 4);
		if (nChannels > 4)
			f.analog[4] = ((buffer[nBytes-7] & 0x0F) << 2) | (buffer[nBytes-8] >> 6);
		if (nChannels > 5)
			f.analog[5] = buffer[nBytes-8] & 0x3F;
	}

   return (int) frames.size();
}

//------------------------------------------------------------------------------------

void BITalino::trigger(const Vbool &digitalOutput)
{
	unsigned char cmd;
	const size_t len = digitalOutput.size();
	
	if (isBitalino2)
	{
		if (len != 0 && len != 2)   throw Exception(Exception::INVALID_PARAMETER);
		
		cmd = 0xB3;   // 1  0  1  1  O2 O1 1  1 - Set digital outputs
	}
	else
	{
		if (len != 0 && len != 4)   throw Exception(Exception::INVALID_PARAMETER);
		
		if (nChannels == 0)   throw Exception(Exception::DEVICE_NOT_IN_ACQUISITION);
		
		cmd = 0x03;   // 0  0  O4 O3 O2 O1 1  1 - Set digital outputs
	}
	
	for(size_t i = 0; i < len; i++)
		if (digitalOutput[i])
			cmd |= (0x04 << i);
		
	send(cmd);
}

//------------------------------------------------------------------------------------

void BITalino::pwm(int pwmOutput)
{
	if (!isBitalino2)    throw Exception(Exception::NOT_SUPPORTED);
	
	if (pwmOutput < 0 || pwmOutput > 255)   throw Exception(Exception::INVALID_PARAMETER);
	
	send((char) 0xA3);
	send(pwmOutput);
}

//------------------------------------------------------------------------------------

BITalino::State BITalino::state(void)
{

//------------------------------------------------------------------------------------
// Byte-aligned structure

#pragma pack(1)

	struct StateX
	{
		unsigned short analog[6];
		unsigned char  batThreshold, portsCRC;
	} statex;

//------------------------------------------------------------------------------------
// Restore default alignment

#pragma pack()

	if (!isBitalino2)    throw Exception(Exception::NOT_SUPPORTED);
	
	if (nChannels != 0)   throw Exception(Exception::DEVICE_NOT_IDLE);
	
	send(0x0B);    // 0  0  0  0  1  0  1  1 - Send device status
	
	if (recv(&statex, sizeof statex) != sizeof statex)    // a timeout has occurred
		throw Exception(Exception::CONTACTING_DEVICE);
		
	if (!checkCRC4((unsigned char *) &statex, sizeof statex))
		throw Exception(Exception::CONTACTING_DEVICE);
	
	State state;
	
	for(int i = 0; i < 6; i++)
		state.analog[i] = statex.analog[i];
	
	for(int i = 0; i < 4; i++)
		state.digital[i] = ((statex.portsCRC & (0x80 >> i)) != 0);

	return state;
}

//------------------------------------------------------------------------------------

const char* BITalino::Exception::getDescription(void)
{
	switch (code)
	{
		case INVALID_ADDRESS:
			return "The specified address is invalid.";
			
		case BT_ADAPTER_NOT_FOUND:
			return "No Bluetooth adapter was found.";

		case DEVICE_NOT_FOUND:
			return "The device could not be found.";

		case CONTACTING_DEVICE:
			return "The computer lost communication with the device.";

		case PORT_COULD_NOT_BE_OPENED:
			return "The communication port does not exist or it is already being used.";

		case PORT_INITIALIZATION:
			return "The communication port could not be initialized.";

		case DEVICE_NOT_IDLE:
			return "The device is not idle.";
			
		case DEVICE_NOT_IN_ACQUISITION:
	        return "The device is not in acquisition mode.";
		
		case INVALID_PARAMETER:
			return "Invalid parameter.";

		case NOT_SUPPORTED:
			return "Operation not supported by the device.";

		default:
			return "Unknown error.";
	}
}

//------------------------------------------------------------------------------------
// BITalino private methods

void BITalino::send(char cmd)
{
	Sleep(150);

//------------------------------------------------------------------------------------
// Windows

#ifdef _WIN32

	if (fd == INVALID_SOCKET)
	{
		DWORD nbytwritten = 0;
		if (!WriteFile(hCom, &cmd, sizeof cmd, &nbytwritten, NULL))
			throw Exception(Exception::CONTACTING_DEVICE);
		if (nbytwritten != sizeof cmd)
			throw Exception(Exception::CONTACTING_DEVICE);
	}
	else
	{
		if (::send(fd, &cmd, sizeof cmd, 0) != sizeof cmd)
			throw Exception(Exception::CONTACTING_DEVICE);
	}


//------------------------------------------------------------------------------------
// Linux or Mac OS

#else

	if (write(fd, &cmd, sizeof cmd) != sizeof cmd)
		throw Exception(Exception::CONTACTING_DEVICE);

#endif
}

//------------------------------------------------------------------------------------

int BITalino::recv(void *data, int nbyttoread)
{

//------------------------------------------------------------------------------------
// Windows

#ifdef _WIN32

	if (fd == INVALID_SOCKET)
	{
		for(int n = 0; n < nbyttoread;)
		{
			DWORD nbytread = 0;
			if (!ReadFile(hCom, (char *) data+n, nbyttoread-n, &nbytread, NULL))
				throw Exception(Exception::CONTACTING_DEVICE);
			if (nbytread == 0)
			{
				DWORD stat;
				if (!GetCommModemStatus(hCom, &stat) || !(stat & MS_DSR_ON))
					throw Exception(Exception::CONTACTING_DEVICE);  // connection is lost
				return n;   // a timeout occurred
			}
			n += nbytread;
		}
		return nbyttoread;
	}
	
#endif

//------------------------------------------------------------------------------------
// Linux or Mac OS
#ifndef _WIN32

	timeval  readtimeout;
	readtimeout.tv_sec = 5;
	readtimeout.tv_usec = 0;

#endif
	
	fd_set   readfds;
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	
	for(int n = 0; n < nbyttoread;)
	{
		int state = select(FD_SETSIZE, &readfds, NULL, NULL, &readtimeout);
		if(state < 0)	 throw Exception(Exception::CONTACTING_DEVICE);
		if (state == 0)   return n;   // a timeout occurred
		
//------------------------------------------------------------------------------------
// Windows

#ifdef _WIN32
	
	int ret = ::recv(fd, (char *) data+n, nbyttoread-n, 0);

//------------------------------------------------------------------------------------
// Linux or Mac OS

#else
	
	ssize_t ret = ::read(fd, (char *) data+n, nbyttoread-n);

#endif

	if(ret <= 0)   throw Exception(Exception::CONTACTING_DEVICE);
	n += ret;
	}
	
	return nbyttoread;
}

//------------------------------------------------------------------------------------

void BITalino::close(void)
{

//------------------------------------------------------------------------------------
// Windows

#ifdef _WIN32

	if (fd == INVALID_SOCKET)
		CloseHandle(hCom);
	else
	{
		closesocket(fd);
		WSACleanup();
	}

//------------------------------------------------------------------------------------
// Linux or Mac OS

#else
	
	::close(fd);

#endif
}

//------------------------------------------------------------------------------------