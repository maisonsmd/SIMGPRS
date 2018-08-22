// SIMGPRS.h
/*
*	TO DO:
*	- Make DTR pin usable in sleep, wakeup functions -> DONE
*	- Add soft DTMF functions
*	- Add phone-call functions
*	- Add keywords file
*/
/*
*	POWER CONSUMTION
*	- IDLE: ~13mA
*	- GPRS data transferring: ~110mA normal, ~115mA if AT+CIPQSEND=1 (quick send)
*	- GPRS IDLE: ~15mA
*	- SLEEP when GPRS is connected: ~3mA normally, some time up ~30mA
*/

#ifndef _SIMGPRS_h
#define _SIMGPRS_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <TimeLib.h>
#include <MemoryFree.h>

#define SIM_SERIAL_TYPE	SoftwareSerial
#define LOG_SERIAL_TYPE HardwareSerial

#if SIM_SERIAL_TYPE == SoftwareSerial
#include <SoftwareSerial.h>
#endif

// The interval between 2 times checking for new message (ms), 0 = disable
#define NEW_MESSAGE_CHECKING_INTERVAL 0
// The interval between 2 times refresh GPRS connection status (ms)
#define REFRESH_GPRS_STATUS_INTERVAL 30000
// Number of tries to send commands
#define DEFAULT_TRIES 1
// Time to wait for commands to complete
#define DEFAULT_TIMEOUT 3000
// The maximum length of receive data, prevent arduino from spending too much RAM on receiving data
#define MAX_RESPONSE_LENGTH 350
// Baud-rate of debugging serial port
//#define DEBUG_BAUD 115200	//Defined in  main code
// Baud-rate of SIM module's serial port
#define SIM_BAUD 19200		//Slow, but safe
// if the number of errors reaches this number, reset the SIM module
#define MAX_ERRORS_TO_RESET 15
// uncomment the line below to allow auto check for new message usually to make sure no message is skipped and delete it after parsing
#define WAIT_FOR_ANY_NEW_MESSAGE
// uncomment the line below to allow inputing commands from terminal
#define ALLOW_TERMINAL_INPUT
// uncomment the line below ignore uppercase characters while comparing
#define IGNORE_SMS_UPPERCASE
// uncomment the line below to enable Debugging
#define ENABLE_LOGGING
// uncomment the line below to allow ECHO
#define ENABLE_ECHO
// uncomment the line below to use DTMF
// #define USE_DTMF //in developing
// uncomment the line below to invert the POWER pin active state, default is Active-HIGH
// #define INVERT_POWER_PIN
// uncomment the line below to invert the RESET pin active state, default is Active-LOW
// #define INVERT_RESET_PIN
// uncomment the line below to invert the DTR pin active state, default is Active-LOW
// #define INVERT_DTR_PIN

#define FS(x) (const __FlashStringHelper*)F(x)
#define DEFAULT_RESPONSE_OK FS("OK")
#define DEFAULT_RESPONSE_ERROR FS("ERROR")

#ifdef ENABLE_LOGGING
#define LOG(x) _loggingPort->print(x)
#define LOGLN(x) _loggingPort->println(x)
#define LOGF(x) _loggingPort->print(F(x))
#define LOGLNF(x) _loggingPort->println(F(x))
#else
#define LOG(x)
#define LOGLN(x)
#define LOGF(x)
#define LOGLNF(x)
#endif // ENABLE_LOGGING

#if defined(ENABLE_ECHO) && defined(ENABLE_LOGGING)
#define ECHO(x) _loggingPort->print(x)
#define ECHOLN(x) _loggingPort->println(x)
#define ECHOF(x) _loggingPort->print(F(x))
#define ECHOLNF(x) _loggingPort->println(F(x))
#else
#define ECHO(x)
#define ECHOLN(x)
#define ECHOF(x)
#define ECHOLNF(x)
#endif // ENABLE_ECHO

#define SEND(x) _port->print(x)
#define AVAILABLE _port->available()
#define READ _port->read()

enum RequestMethod
{
	GET,
	POST
};

enum GprsStatus
{
	CONNECTED,
	DISCONNECTED
};

enum NetworkStatus
{
	REGISTERED,
	UNREGISTERD,
	ERROR
};

enum PowerStatus
{
	// Information about power can be found at http://www.rhydolabz.com/documents/gps_gsm/sim900_rs232_gsm_modem_opn.pdf , page 6
	POWERD_ON, // Hardware power on, enter full function state
	HARD_POWERD_OFF, // Hardware power off, doesn't consume any current
	PRE_POWERD_OFF, // AT+CPOWD=1	~13mA consumed, can't use AT commands, this help the module save data before hard power off, the module automatically wakeup after about 30s (!)
	MINIMUM_FUNC, // AT+CFUN=0	~13mA consumed, disable RF, can use AT commands
	FULL_FUNC, // AT+CFUN=1	~13mA consumed, can use everything
	FLIGHT, // AT+CFUN=4	disable RF, can use AT commands
	SLEEP, // AT+CSCLK=1	~3mA consumed, can display incoming phone-call and incoming SMS message indication, can't use AT commands (so cant answer the incoming call!), DTR pin required to wake up the module
	AUTO_SLEEP // AT+CSCLK=2	~3mA consumed, automatically enter sleep state when no activity has been working for about 7s, auto wakeup when call any commands TWICE
};

enum MicGain
{
	MIC_GAIN_0dB = 0,
	MIC_GAIN_2dB = 1,
	MIC_GAIN_3dB = 2,
	MIC_GAIN_5p5dB = 3,
	MIC_GAIN_6dB = 4,
	MIC_GAIN_8dB = 5,
	MIC_GAIN_9dB = 6,
	MIC_GAIN_11dB = 7,
	MIC_GAIN_12dB = 8,
	MIC_GAIN_14dB = 9,
	MIC_GAIN_15dB = 10,
	MIC_GAIN_17dB = 11,
	MIC_GAIN_18dB = 12,
	MIC_GAIN_20dB = 13,
	MIC_GAIN_21dB = 14,
	MIC_GAIN_23dB = 15
};

struct MessageCommand
{
	const __FlashStringHelper * message;
	void(*function) (String *, String *) = nullptr;
};

class SIMGPRS
{
protected:
	SIM_SERIAL_TYPE * _port;

#ifdef ENABLE_LOGGING
	LOG_SERIAL_TYPE * _loggingPort;
#endif // ENABLE_LOGGING

	uint8_t _msgCmdCounter = 0;
	MessageCommand _msgCmdContainer[1];
	void(*_defaultMessageFunction) (String *, String *) = nullptr; // This function runs when the received SMS doesn't match any command added
	// void(* _streamFunction)(String *, String *) = nullptr;		//This function runs when the server response to the stream
	void(*_debugFunction)(String *) = nullptr;
	void(*_incomingCallHandlerFunction)(String *, boolean) = nullptr;
	boolean _isNewMessageChecked = false;
	PowerStatus _POWER_STATUS;
	NetworkStatus _NET_STATUS;
	GprsStatus _GPRS_STATUS;
	int8_t _POWER_PIN = -1;
	int8_t _RESET_PIN = -1;
	int8_t _DTR_PIN = -1;
	int8_t _timeZone = 7;
public:
	uint8_t _errorCounter = 0; // Count the errors, if reaches a predefined number, reset the SIM module

#ifdef ENABLE_LOGGING
	SIMGPRS(SIM_SERIAL_TYPE * p, LOG_SERIAL_TYPE * dp) :_port(p), _loggingPort(dp) {	};
#else
	SIMGPRS(SIM_SERIAL_TYPE * p) :_port(p) {	};
#endif

	void init();
	void attachPowerPin(int8_t pin);
	void attachResetPin(int8_t pin);
	void attachDtrPin(int8_t pin);
	void hardPowerOff();
	void hardPowerOn();
	void hardReset();
	boolean reset();
	boolean sleep();
	boolean wakeUp();
	PowerStatus powerStatus();
	NetworkStatus networkStatus();
	GprsStatus gprsStatus();
	boolean requestUSSD(String ussd, String * responseText = nullptr);
private:
	boolean waitForResponse_str(String command, uint8_t tries, uint32_t timeout, const __FlashStringHelper * okText, const __FlashStringHelper * errorText, String * responseText);
	boolean waitForResponse_F(const __FlashStringHelper * command, uint8_t tries, uint32_t timeout, const __FlashStringHelper * okText, const __FlashStringHelper * errorText, String * responseText);
public:
	void attachDebugFunction(void(*function)(String *));
	void run();
	String getProviderName();
	int8_t getSignalStrength(int8_t * percent = nullptr); // unit: %
	int getBatteryLevel(uint8_t * percent = nullptr); // unit: mV [+percent]
	time_t getRTC_Time();
	boolean setRTC_Time(time_t time);
#pragma region GPRS
	boolean synchronizeClockToNetworkTime(int8_t timezone);
	boolean getLocation(float * latitude, float * longitude);
	boolean keepConnectionAlive();
	boolean attachGPRS(String APN, String user, String password); // For using HTTP only, doesn't effect TCP/UDP
	boolean connect(); // For using HTTP only, doesn't effect TCP/UDP
	boolean disconnect(); // For using HTTP only, doesn't effect TCP/UDP
	GprsStatus connectionStatus(); // For using HTTP only, doesn't effect TCP/UDP
	boolean requestHTTP(RequestMethod method, String * host, String * parameters, uint16_t* resultCode = nullptr, String* serviceResponse = nullptr);
	// boolean requestHTTP(RequestMethod method, String host, String * parameter,uint16_t * resultCode = nullptr, String * serviceResponse = nullptr);
	// boolean stream(const __FlashStringHelper * host, String * dataToSend, void(streamFunction(String *, String *)));
#pragma endregion
#pragma region SMS
	boolean deleteAllMessages();
	boolean deleteMessage(uint8_t index);
	boolean readMessage(String * message, String * sender, uint8_t index = 1);
	boolean sendSMS(String * phoneNumber, String * message);
	void addMessageCommand(const __FlashStringHelper * message, void(*function) (String *, String *));
	void setDefaultMessageFunction(void(*function) (String *, String *));
#pragma endregion
#pragma region PHONECALL
	boolean setCallAutoAnswer(uint8_t numberOfRing = 1); // =0 => disable auto answer
	boolean answerCall();
	boolean rejectCall();
	boolean call(String * phoneNumber);
	boolean setMicrophoneGainLevel(MicGain gain);
	void attachIncomingCallHandlerFunction(void(*function)(String *, boolean));
#pragma endregion
#pragma region DTMF
	// Nothing here
#pragma endregion
private:
	// Code below is low-level interface
	void _checkCallingStatus(String * str);
	void _clearBuffer();
	void _pullDtr();
	void _releaseDtr();
	void _scanMessagesContent();
	void _waitForNextChar();
	PowerStatus _sleepStatus();
	boolean _sendCmd_F(const __FlashStringHelper * cmd, uint8_t count);
	boolean _sendCmd_strptr(String * cmd, uint8_t count);
	boolean _sendCmd_str(String cmd, uint8_t count);
	void _checkNewMessageNextTime();
	boolean _waitForResponse(uint32_t * timeout, const __FlashStringHelper * okText, const __FlashStringHelper * errorText, String * responseText);

public:
	boolean findBaud()
	{
		LOGLNF("FINDING BAUD...");
		_pullDtr();				//in case the SIM is sleeping
#ifdef ENABLE_LOGGING
		//_loggingPort->begin(DEBUG_BAUD);	//called in main code
#endif
		delay(1000);
		uint32_t baudArray[] = { 38400, 115200, 9600, 57600, 19200, 14400, 4800, 2400, 1200, 28800 };
		char c;
		for (uint8_t i = 0; i < sizeof(baudArray) / sizeof(baudArray[0]); i++) {
			while (AVAILABLE) {
				READ;
				uint32_t u = micros();
				while (micros() - u < 5000)
					if (AVAILABLE)
						continue;
			}
			LOGF("\nTRYING ");
			LOG(baudArray[i]);
			LOGF("...\nECHO:  ");
			_port->begin(baudArray[i]);
			delay(100);
			SEND(F("AT+IPR="));
			SEND(SIM_BAUD);
			SEND(F("\r\n"));
			delay(100);
			String s;
			uint32_t startTick = millis();
			while (AVAILABLE) {
				/*if (millis() - startTick > 2000)
					return false;*/
				while (AVAILABLE) {
					/*if (millis() - startTick > 2000)
						return false;*/
					c = READ;
					LOG(c);
					if (c == '\n') LOG("       ");
					if (s.length() < 30)
						s += c;
				}
				if (s.indexOf(F("OK")) >= 0
					|| s.indexOf(F("FUN")) >= 0 // +CFUN
					|| s.indexOf(F("PIN")) >= 0 // +CPIN
					|| s.indexOf(F("all")) >= 0 // Call
					|| s.indexOf(F("eady")) >= 0 // Ready
					|| s.indexOf(F("RDY")) >= 0 // RDY
					|| s.indexOf(F("SMS")) >= 0 // SMS
					|| s.indexOf(F("\r\n")) >= 0) {
					_port->begin(SIM_BAUD);
					LOGLN();
					_POWER_STATUS = _sleepStatus();
					if (_POWER_STATUS != PowerStatus::FULL_FUNC) {
						LOGLNF("SIM IS SLEEPING, WAKING UP...");
						wakeUp();
					}
					_releaseDtr();
					LOGF("\nFOUND BAUD: ");
					LOG(baudArray[i]);
					LOGLNF("");
					if (baudArray[i] == SIM_BAUD)
						return true;
					LOGF("RESET TO ");
					LOGLN(SIM_BAUD);
					delay(50);
					while (AVAILABLE)
						READ;
					return true;
				}
			}
		}
		LOGLNF("\nCAN'T SET BAUD\n");
		_errorCounter++;
		LOGF("NUMBER OF ERRORS TO RESET: ");
		LOGLN(String(_errorCounter) + "/" + String(MAX_ERRORS_TO_RESET) + "\n");
		if (_errorCounter >= MAX_ERRORS_TO_RESET) {
			_errorCounter = 0;
			LOGLNF("ERR: SIM IS NOT RESPONSIBLE, TRYING HARD RESET...\n");
			hardReset();
		}
		return false;
	}
};
boolean _timediff(uint32_t * timer, uint32_t interval);
#endif