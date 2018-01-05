// 
// 
// 
#include "SIMGPRS.h"

void SIMGPRS::init()
{
	LOGLNF("\nINITIALIZING SIM...\n");
	_POWER_STATUS = PowerStatus::FULL_FUNC;
	_NET_STATUS = NetworkStatus::UNREGISTERD;
	_clearBuffer();
	if (!waitForResponse_F(F("AT+CFUN=1"), 5, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
	{
		LOGLNF("ERR: UNABLE TO SET FULL FUNCTIONALITY\n");
		_NET_STATUS = NetworkStatus::ERROR;
	}
	if (!waitForResponse_F(F("AT+CREG?"), 10, DEFAULT_TIMEOUT, F("0,1"), DEFAULT_RESPONSE_ERROR, nullptr))
	{
		if (_NET_STATUS != ERROR)
			_NET_STATUS = NetworkStatus::UNREGISTERD;
		LOGLNF("ERR: UNABLE TO REGISTER TO NETWORK\n");
	}
	_NET_STATUS = NetworkStatus::REGISTERED;
	waitForResponse_F(F("AT+CSCLK=0"), 2, 500, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); // Wake up
	waitForResponse_F(F("AT&F0"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); // reset default configuration
	waitForResponse_F(F("ATE1"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); //
	waitForResponse_F(F("AT+CMEE=2"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); // Better error report
	waitForResponse_F(F("AT+CMGF=1"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); //
	waitForResponse_F(F("AT+CSCS=\"GSM\""), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); //
	waitForResponse_F(F("AT+CNMI=2,1,0,0,0"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); // Turn on sms indicator, turn off showing full message
	waitForResponse_F(F("AT+CUSD=0"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); //
	waitForResponse_F(F("AT+CLIP=1"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); // Show caller phone number
	waitForResponse_F(F("AT+CIPHEAD=1"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); // Show +IPD indicator when the server responses
	waitForResponse_F(F("AT+HTTPINIT"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); //
	waitForResponse_F(F("AT+HTTPPARA=\"CID\",1"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); //
	waitForResponse_F(F("AT+HTTPPARA=\"UA\",\"maisonsmd (c)\""), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); //
	waitForResponse_F(F("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\""), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); //
	connectionStatus(); // Refresh connection status
	LOGLNF("INIT DONE\n");
}

void SIMGPRS::attachPowerPin(int8_t pin)
{
	_POWER_PIN = pin;
	pinMode(_POWER_PIN, OUTPUT);
	hardPowerOff();
}

void SIMGPRS::attachResetPin(int8_t pin)
{
	_RESET_PIN = pin;
	pinMode(_RESET_PIN, INPUT); // release
}

void SIMGPRS::attachDtrPin(int8_t pin)
{
	_DTR_PIN = pin;
	pinMode(_DTR_PIN, INPUT); // release
}

void SIMGPRS::hardPowerOff()
{

#ifdef INVERT_POWER_PIN
	digitalWrite(_POWER_PIN, HIGH);
#else
	digitalWrite(_POWER_PIN, LOW);
#endif

}

void SIMGPRS::hardPowerOn()
{

#ifdef INVERT_POWER_PIN
	digitalWrite(_POWER_PIN, LOW);
#else
	digitalWrite(_POWER_PIN, HIGH);
#endif

}

void SIMGPRS::hardReset()
{

#ifdef INVERT_POWER_PIN
	pinMode(_RESET_PIN, OUTPUT);
	digitalWrite(_RESET_PIN, HIGH);
	delay(100);
	pinMode(_RESET_PIN, INPUT);
#else
	pinMode(_RESET_PIN, OUTPUT);
	digitalWrite(_RESET_PIN, LOW);
	delay(100);
	pinMode(_RESET_PIN, INPUT);
#endif

}

boolean SIMGPRS::reset()
{
	if (_POWER_STATUS == PowerStatus::FULL_FUNC || _POWER_STATUS == MINIMUM_FUNC || _POWER_STATUS == FLIGHT)
	{
		if (waitForResponse_F(F("AT+CFUN=1,1"), 2, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
		{
			_POWER_STATUS = PowerStatus::FULL_FUNC;
			LOGLNF("SOFT RESET DONE\n");
			return true;
		}
		else
		{
			LOGLNF("ERR: SOFT RESET FAILED\n");
			return false;
		}
	}
	else
	{
		LOGLNF("ERR: CAN'T RESET DUE TO BEING UNABLE TO SENT CMD\n");
		return false;
	}
}

boolean SIMGPRS::sleep()
{
	if (_POWER_STATUS == PowerStatus::SLEEP || _POWER_STATUS == PowerStatus::AUTO_SLEEP)
	{
		LOGLNF("ERR: ALREADY SLEPT\n");
		return true;
	}
	//if (waitForResponse_F(F("AT+CSCLK=2"), 3, 1000, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
	if (waitForResponse_F(F("AT+CSCLK=1"), 3, 1000, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
	{
		//_POWER_STATUS = PowerStatus::AUTO_SLEEP;
		_POWER_STATUS = PowerStatus::SLEEP;
		LOGLNF("SLEEP DONE\n");
		return true;
	}
	LOGLNF("ERR: SLEEP FAILED\n");
	return false;
}

boolean SIMGPRS::wakeUp()
{
	if (_POWER_STATUS != PowerStatus::FULL_FUNC)
	{
		PowerStatus tempPowerStatus = _POWER_STATUS;
		_POWER_STATUS = PowerStatus::FULL_FUNC;
		delay(200);
		_pullDtr();
		delay(200);
		if (waitForResponse_str("AT+CSCLK=0", 15, 100, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr)
			&& waitForResponse_F(F("AT+CFUN=1"), 2, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
		{
			LOGLNF("WAKE UP DONE\n");
			_releaseDtr();
			return true;
		}
		_POWER_STATUS = tempPowerStatus;
		LOGLNF("ERR: WAKE UP FAILED\n");
		_releaseDtr();
		return false;
	}
	LOGLNF("ERR: NOT IN SLEEP MODE, CAN'T WAKE UP\n");
	_releaseDtr();
	return true;
}

PowerStatus SIMGPRS::powerStatus()
{
	return _POWER_STATUS;
}

NetworkStatus SIMGPRS::networkStatus()
{
	return _NET_STATUS;
}

GprsStatus SIMGPRS::gprsStatus()
{
	return _GPRS_STATUS;
}

boolean SIMGPRS::requestUSSD(String ussd, String * responseText)
{
	ussd = "AT+CUSD=1,\"" + ussd + "\"";
	if (waitForResponse_str(ussd, DEFAULT_TRIES, 3 * DEFAULT_TIMEOUT, F("+CUSD:"), DEFAULT_RESPONSE_ERROR, responseText))
	{
		if (responseText != nullptr)
			* responseText = responseText->substring(responseText->indexOf(F(", \"")) + 3, responseText->lastIndexOf(F("\"")));
		waitForResponse_F(F("AT+CUSD=0"), 2, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr);
		return true;
	}
	LOGLNF("ERR: REQUEST USSD FAILED\n");
	waitForResponse_F(F("AT+CUSD=0"), 2, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr);
	return false;
}

boolean SIMGPRS::waitForResponse_str(String command, uint8_t tries, uint32_t timeout, const __FlashStringHelper * okText, const __FlashStringHelper * errorText, String * responseText)
{
	uint8_t count = 1;
	do
	{
		if (!_sendCmd_strptr(&command, count))
			return false;
		if (_waitForResponse(&timeout, okText, errorText, responseText))
		{
			_errorCounter = 0;
			return true;
		}
		else
		{
			_errorCounter++;
			LOGLN("NUMBER OF ERRORS TO RESET: " + String(_errorCounter) + "/" + String(MAX_ERRORS_TO_RESET) + "\n");
			if (_errorCounter >= MAX_ERRORS_TO_RESET)
			{
				_errorCounter = 0;
				LOGLNF("ERR: SIM IS NOT RESPONSIBLE, TRYING HARD RESET...\n");
				hardReset();
				init();
				return false;
			}
		}
	} while (count++ < tries);
	LOGLNF("ERR: MAX TRIES REACHED\n");
	return false;
}

boolean SIMGPRS::waitForResponse_F(const __FlashStringHelper * command, uint8_t tries, uint32_t timeout, const __FlashStringHelper * okText, const __FlashStringHelper * errorText, String * responseText)
{
	uint8_t count = 1;
	do
	{
		if (command != nullptr) {
			if (!_sendCmd_F(command, count))
				return false;
		}
		if (_waitForResponse(&timeout, okText, errorText, responseText)) {
			_errorCounter = 0;
			return true;
		}
		else {
			_errorCounter++;
			LOGLN("NUMBER OF ERRORS TO RESET: " + String(_errorCounter) + "/" + String(MAX_ERRORS_TO_RESET) + "\n");
			if (_errorCounter >= MAX_ERRORS_TO_RESET) {
				_errorCounter = 0;
				LOGLNF("ERR: SIM IS NOT RESPONSIBLE, TRYING HARD RESET...\n");
				hardReset();
				init();
				return false;
			}
		}
	} while (count++ < tries);
	LOGLNF("ERR: MAX TRIES REACHED\n");
	return false;
}

void SIMGPRS::setDebugFunction(void(*function)(String *))
{
	_debugFunction = function;
}

void SIMGPRS::waitForAnything()
{
	String strSimResp = "", strDebug = "";

#ifdef ENABLE_LOGGING

#ifdef ALLOW_TERMINAL_INPUT // Forward command user typed to 'port' (SIM module)
	if (_loggingPort->available()) {
		LOGF("TYPED: ");
		while (_loggingPort->available()) {
			SLOW_DOWN;
			strDebug += char(_loggingPort->read());
		}
		strDebug.replace('~', 0x1A);
		//strDebug.trim();
		LOGLN(strDebug);
		//strDebug += "\r\n";
		if (strDebug.charAt(0) != '>')
			SEND(strDebug);
		else {
			strDebug.remove(0, 1);
			if (_debugFunction != nullptr)
				_debugFunction(&strDebug);
		}
	}
#endif // ALLOW_TERMINAL_INPUT

#endif // ENABLE_LOGGING

	if (AVAILABLE) {

#ifdef ENABLE_ECHO
		ECHOF("ECHO:  ");
#endif // ENABLE_ECHO

		while (AVAILABLE) {
			SLOW_DOWN;
			char c = READ;
			if (strSimResp.length() < MAX_RESPONSE_LENGTH)
				strSimResp += c;
			if (strSimResp.length() == MAX_RESPONSE_LENGTH - 1) {
				LOGLNF("WRN: MAX RESPONSE SIZE REACHED\n");
#ifndef ENABLE_ECHO
				_clearBuffer();
				break;
#endif // !ENABLE_ECHO
			}
#ifdef ENABLE_ECHO // Echo what SIM response to 'debugPort'
			ECHO(c);
			if (c == '\n')
				ECHOF("       "); // Just to align the text
#endif // ENABLE_ECHO
		}
		ECHOLNF("");
	}
	static uint32_t entry = 0;
	if ((strSimResp.indexOf(F("+CMTI:")) >= 0)
		|| (NEW_MESSAGE_CHECKING_INTERVAL > 0 && _timediff(&entry, NEW_MESSAGE_CHECKING_INTERVAL) && _POWER_STATUS == PowerStatus::FULL_FUNC)
		|| isNewMessageChecked == false) {
		PowerStatus lastPowerStatus = powerStatus();
		if (lastPowerStatus != PowerStatus::FULL_FUNC) {
			wakeUp();
			delay(1000);				//SIM can't send SMS fast right after waking up (costs ~28s, if delay a little bit, just costs ~4s (normal))
		}
		_scanMessagesContent();
		deleteAllMessages(); // clean the message store because we still read the message whose index is 1 next time
		if (lastPowerStatus != PowerStatus::FULL_FUNC) sleep();
	}
}

String SIMGPRS::getProviderName()
{
	String name = "";
	if (!waitForResponse_F(F("AT+CSPN?"), DEFAULT_TRIES, DEFAULT_TIMEOUT, F("+CSPN:"), DEFAULT_RESPONSE_ERROR, &name))
		return "";
	name = name.substring(name.indexOf(F("\"")) + 1, name.lastIndexOf(F("\"")));
	return name;
}

int8_t SIMGPRS::getSignalStrength(int8_t * dBm)
{
	String response = "";
	int8_t strengthIndex = 0, percent = 0, strengthdBm = 0;
	if (waitForResponse_F(F("AT+CSQ"), DEFAULT_TRIES, DEFAULT_TIMEOUT, F("+CSQ:"), DEFAULT_RESPONSE_ERROR, &response))
		strengthIndex = uint8_t(response.substring(response.indexOf(":") + 2, response.indexOf(",")).toInt());
	switch (strengthIndex)
	{
	case 0:
		strengthdBm = -115;
		break;
	case 1:
		strengthdBm = -111;
		break;
	case 31:
		strengthdBm = -52;
		break;
	case 99:
		strengthdBm = 0;
		break;
	default:
		strengthdBm = map(strengthIndex, 2, 30, -110, -54);
		break;
	}
	if (strengthIndex == 99)
		percent = 0;
	else
		percent = map(strengthIndex, 0, 31, 0, 100);
	percent = constrain(percent, 0, 100);
	if (dBm != nullptr)
		* dBm = strengthdBm;
	return percent;
}

int SIMGPRS::getBatteryLevel(uint8_t * percent)
{
	String response = "";
	if (waitForResponse_F(F("AT+CBC"), DEFAULT_TRIES, DEFAULT_TIMEOUT, F("+CBC:"), DEFAULT_RESPONSE_ERROR, &response))
	{
		if (percent != nullptr)
			* percent = uint8_t(response.substring(response.indexOf(",") + 1, response.lastIndexOf(",")).toInt());
		return response.substring(response.lastIndexOf(",") + 1, response.lastIndexOf(",") + 5).toInt(); // millivol
	}
	return 0;
}

boolean SIMGPRS::getTime(uint8_t * hour, uint8_t * minute, uint8_t * second, uint8_t * day, uint8_t * month, uint16_t * year)
{
	String response = "";
	if (!waitForResponse_F(F("AT+CCLK?"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, &response))
		return false;
	int8_t basePosition = response.indexOf(F("+CCLK:"));
	*hour = uint8_t(response.substring(basePosition + 17, basePosition + 19).toInt());
	*minute = uint8_t(response.substring(basePosition + 20, basePosition + 22).toInt());
	*second = uint8_t(response.substring(basePosition + 23, basePosition + 25).toInt());
	if (day != nullptr)
		* day = uint8_t(response.substring(basePosition + 14, basePosition + 16).toInt());
	if (month != nullptr)
		* month = uint8_t(response.substring(basePosition + 11, basePosition - 13).toInt());
	if (year != nullptr)
		* year = uint16_t(response.substring(basePosition + 8, basePosition + 10).toInt()) + 2000;
	return true;
}

boolean SIMGPRS::setTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t day, uint8_t month, uint16_t year)
{
	String dateTime = "AT+CCLK=\"";
	dateTime += String(year - 2000) + "/" + (month < 10 ? "0" : "") + String(month) + "/" + (day < 10 ? "0" : "") + String(day);
	dateTime += ",";
	dateTime += (hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute) + ":" + (second < 10 ? "0" : "") + String(second);
	dateTime += "+00\"";
	_sendCmd_strptr(&dateTime, 1);
	if (!waitForResponse_F(nullptr, DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
		return false;
	return true;
}

boolean SIMGPRS::synchronizeClockToNetworkTime(int8_t timezone)
{
	_checkNewMessageNextTime();
	if (!keepConnectionAlive())
	{
		LOGLNF("ERR: UNABLE TO ESTABLISH CONNECTION");
		return false;
	}
	waitForResponse_F(F("AT+CNTPCID=1"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr);
	SEND(F("AT+CNTP=\"202.120.2.101\","));
	SEND(timezone * 4);
	SEND(F("\r\n"));
	if (!waitForResponse_F(nullptr, 3 * DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
		return false;
	if (!waitForResponse_F(F("AT+CNTP"), DEFAULT_TRIES, 10 * DEFAULT_TIMEOUT, F(":"), DEFAULT_RESPONSE_ERROR, nullptr))
		return false;
	waitForResponse_F(F("AT+CCLK?"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr);
	return true;
}

boolean SIMGPRS::getLocation(double * latitude, double * longitude)
{
	_checkNewMessageNextTime();
	if (!keepConnectionAlive())
	{
		LOGLNF("ERR: UNABLE TO ESTABLISH CONNECTION");
		return false;
	}
	String result = "";
	if (!waitForResponse_F(F("AT+CIPGSMLOC=1,1"), DEFAULT_TRIES, 3 * DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, &result))
		return false;
	result = result.substring(result.indexOf(":"), result.lastIndexOf(","));
	result = result.substring(result.indexOf(",") + 1, result.lastIndexOf(","));
	*longitude = (result.substring(0, result.indexOf(","))).toDouble();
	*latitude = (result.substring(result.indexOf(",") + 1, result.length())).toDouble();
	return true;
}

boolean SIMGPRS::keepConnectionAlive()
{
	if (connectionStatus() != GprsStatus::CONNECTED)
		connect();
	if (connectionStatus() != GprsStatus::CONNECTED)
		return false;
	else
		return true;
}

boolean SIMGPRS::attachGPRS(String APN, String user, String password)
{
	if (!waitForResponse_F(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
		return false;
	if (!waitForResponse_str("AT+SAPBR=3,1,\"APN\",\"" + APN + "\"", DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
		return false;
	if (user != nullptr)
		if (!waitForResponse_str("AT+SAPBR=3,1,\"USER\",\"" + user + "\"", DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
			return false;
	if (password != nullptr)
		if (!waitForResponse_str("AT+SAPBR=3,1,\"PWD\",\"" + password + "\"", DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
			return false;
}

boolean SIMGPRS::connect()
{
	if (connectionStatus() == GprsStatus::CONNECTED)
	{
		LOGLNF("WRN: GPRS ALREADY CONNECTED");
		return true;
	}
	if (!waitForResponse_F(F("AT+SAPBR=1,1"), 2, 5 * DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
	{
		_GPRS_STATUS = GprsStatus::DISCONNECTED;
		LOGLNF("ERR: CONNECT GPRS FAILED");
		return false;
	}
	LOGLNF("CONNECTED GPRS");
	_GPRS_STATUS = GprsStatus::CONNECTED;
	waitForResponse_F(F("AT+SAPBR=2,1"), 2, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr); // display IP address
	return true;
}

boolean SIMGPRS::disconnect()
{
	if (!waitForResponse_F(F("AT+SAPBR=0,1"), 2, 2 * DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
		return false;
	LOGLNF("DISCONECTED GPRS");
	_GPRS_STATUS = GprsStatus::DISCONNECTED;
	return true;
}

GprsStatus SIMGPRS::connectionStatus()
{
	static uint32_t entry = 0;
	static boolean firstTimeCalled = true;
	if (!_timediff(&entry, REFRESH_GPRS_STATUS_INTERVAL) && !firstTimeCalled)
		return _GPRS_STATUS;
	//if (!_timediff(&entry, REFRESH_GPRS_STATUS_INTERVAL) && !firstTimeCalled) return _GPRS_STATUS;
	firstTimeCalled = false;
	LOGLNF("CHECKING GPRS STATUS...");
	String resp = "";
	if (waitForResponse_F(F("AT+SAPBR=2,1"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, &resp))
	{
		resp = resp.substring(resp.indexOf("\"") + 1, resp.lastIndexOf("\""));
		if (resp.indexOf("0.0.0.0") >= 0)
			_GPRS_STATUS = GprsStatus::DISCONNECTED;
		else
			if (resp.length() > 7)
				_GPRS_STATUS = GprsStatus::CONNECTED;
	}
	else
		_GPRS_STATUS = GprsStatus::DISCONNECTED;
	LOGF("CONNECTION STATUS: ");
	switch (_GPRS_STATUS)
	{
	case CONNECTED:
		LOGLNF("CONNECTED");
		break;
	case DISCONNECTED:
		LOGLNF("DISCONNECTED");
		break;
	}
	return _GPRS_STATUS;
}

boolean SIMGPRS::requestHTTP(RequestMethod method, String * host, String * parameters, uint16_t * resultCode, String * serviceResponse)
{
	_checkNewMessageNextTime();
	if (resultCode != nullptr)
		* resultCode = 0;
	if (serviceResponse != nullptr)
		* serviceResponse = "";
	if (method == RequestMethod::GET)
		LOGF("GETTING FROM ");
	else
		LOGF("POSTING TO ");
	LOG(*host);
	LOGF(" WITH PARAMETERS: ");
	LOGLN(*parameters);
	if (!keepConnectionAlive())
	{
		LOGLNF("ERR: UNABLE TO ESTABLISH CONNECTION");
		return false;
	}
	SEND(F("AT+HTTPPARA=\"URL\",\""));
	SEND(*host);
	if (method == RequestMethod::GET && parameters != nullptr)
	{
		if (*parameters != "")
		{
			SEND("?");
			SEND(*parameters);
		}
	}
	SEND(F("\"\r\n"));
	if (!waitForResponse_F(nullptr, 1, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr))
		return false;
	if (method == RequestMethod::POST)
	{
		if (parameters == nullptr)
		{
			LOGLNF("ERR: ARGUMENT LIST IS EMPTY");
			return false;
		}
		else
			if (*parameters == "")
			{
				LOGLNF("ERR: ARGUMENT LIST IS EMPTY");
				return false;
			}
		SEND(F("AT+HTTPDATA="));
		SEND(String(parameters->length()));
		SEND(F(","));
		SEND(String(DEFAULT_TIMEOUT));
		SEND(F("\r\n"));
		if (!waitForResponse_F(nullptr, 1, DEFAULT_TIMEOUT, F("DOWNLOAD"), DEFAULT_RESPONSE_ERROR, nullptr))
			return false;
		SEND(*parameters);
		SEND(F("\r\n"));
		waitForResponse_F(nullptr, 1, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr);
	}
	String result = "";
	boolean successed = false;
	if (method == RequestMethod::GET)
		successed = waitForResponse_F(F("AT+HTTPACTION=0"), 1, 30000, F("+HTTPACTION:"), DEFAULT_RESPONSE_ERROR, &result);
	else
		successed = waitForResponse_F(F("AT+HTTPACTION=1"), 1, 30000, F("+HTTPACTION:"), DEFAULT_RESPONSE_ERROR, &result);
	if (!successed) {
		LOGLNF("ERR: REQUEST FAILLED");
		return false;
	}
	if (resultCode != nullptr)
		* resultCode = uint16_t(result.substring(result.indexOf(",") + 1, result.lastIndexOf(",")).toInt());
	result = "";
	if (serviceResponse != nullptr) {
		if (waitForResponse_F(F("AT+HTTPREAD"), 1, DEFAULT_TIMEOUT, F("+HTTPREAD:"), DEFAULT_RESPONSE_ERROR, &result)) {
			result = result.substring(result.indexOf(F(":")), result.lastIndexOf(F("\r\nOK")));
			*serviceResponse = result.substring(result.indexOf(F("\r\n")) + 2, result.length());
			return true;
		}
	}
	return true;
}

boolean SIMGPRS::deleteAllMessages()
{
	if (waitForResponse_F(F("AT+CMGD=1,4"), 2, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr)) {
		LOGLNF("DEL ALL MESSAGES DONE\n");
		return true;
	}
	LOGLNF("ERR: DEL ALL MESSAGES FAILLED\n");
	return true;
}

boolean SIMGPRS::deleteMessage(uint8_t index)
{
	if (!_sendCmd_str("AT+CMGD=" + String(index), 1))
		return false;
	if (waitForResponse_F(nullptr, DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr)) {
		LOGF("DEL MESSAGE AT ");
		LOG(index);
		LOGLNF(" DONE\n");
		return true;
	}
	LOGF("DEL MESSAGE AT ");
	LOG(index);
	LOGLNF(" FAILLED\n");
	return true;
}

boolean SIMGPRS::readMessage(String * message, String * sender, uint8_t index)
{
	*message = "";
	*sender = "";
	if (!_sendCmd_str("AT+CMGR=" + String(index), 1))
		return false;
	if (!waitForResponse_F(nullptr, 1, 500, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, message))	//Do not wait for "OK", it won't appear in long message
	{
		if (message->indexOf(F("+CMGR:")) < 0)	//If "OK" won't appear, this signal means we have the message
			return false;
	}
	if (message->indexOf(F("+CMGR:")) < 0)
		return false;
	if (message->lastIndexOf(F("\r\nOK")) > 0)
		*message = message->substring(message->indexOf(F(",")) + 1, message->lastIndexOf(F("\r\nOK")));
	else if (message->lastIndexOf(F("\r\n")) > 0)
		*message = message->substring(message->indexOf(F(",")) + 1, message->lastIndexOf(F("\r\n")));
	else
		*message = message->substring(message->indexOf(F(",")) + 1, message->length());
	*sender = message->substring(message->indexOf(F("\"")) + 1, message->indexOf(F(",")) - 1);
	*message = message->substring(message->lastIndexOf(F("\"")) + 3, message->length());
	return true;
}

boolean SIMGPRS::sendSMS(String * phoneNumber, String * message)
{
	_clearBuffer();
	LOGF("SENDING \"");
	LOG(*message);
	LOGF("\" TO \"");
	LOG(*phoneNumber);
	LOGLNF("\"");
	if (phoneNumber->charAt(0) != '+') {
		LOGLNF("PHONENUM CORRUPTED, ARE YOU TESTING?");
		return;
	}
	if (!waitForResponse_str("AT+CMGS=\"" + *phoneNumber + "\"", DEFAULT_TRIES, DEFAULT_TIMEOUT, F(">"), DEFAULT_RESPONSE_ERROR, nullptr)) {
		LOGLNF("ERR: SENDING FAILLED\n");
		return false;
	}
	SEND(*message);
	SEND(char(0x1A));
	if (waitForResponse_F(nullptr, DEFAULT_TRIES, DEFAULT_TIMEOUT * 11, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, nullptr)) {
		LOGLNF("SENDING DONE\n");
		return true;
	}
	return false;
}

void SIMGPRS::addMessageCommand(const __FlashStringHelper * message, void(*function) (String *, String *))
{
	_msgCmdCounter++;
	MessageCommand * newMsgCmdContainer = new MessageCommand[_msgCmdCounter];
	for (uint8_t i = 0; i < _msgCmdCounter - 1; i++)
		newMsgCmdContainer[i] = _msgCmdContainer[i];
	if (_msgCmdContainer != nullptr)
		delete[] _msgCmdContainer;
	_msgCmdContainer = newMsgCmdContainer;
	_msgCmdContainer[_msgCmdCounter - 1].message = message;
	_msgCmdContainer[_msgCmdCounter - 1].function = function;
}

void SIMGPRS::setDefaultMessageFunction(void(*function) (String *, String *))
{
	_defaultMessageFunction = function;
}

boolean SIMGPRS::enableCallAutoAnswer(uint8_t numberOfRing)
{
	return false;
}

boolean SIMGPRS::disableCallAutoAnswer()
{
	return false;
}

boolean SIMGPRS::answerCall()
{
	return false;
}

boolean SIMGPRS::rejectCall()
{
	return false;
}

boolean SIMGPRS::call(String * phoneNumber)
{
	return false;
}

boolean SIMGPRS::setMicrophoneGainLevel(MicGain gain)
{
	return false;
}

void SIMGPRS::_clearBuffer()
{
	while (AVAILABLE)
		READ;
}

void SIMGPRS::_pullDtr()
{
	pinMode(_DTR_PIN, OUTPUT);
#ifdef INVERT_DTR_PIN
	digitalWrite(_DTR_PIN, HIGH);
#else
	digitalWrite(_DTR_PIN, LOW);
#endif // INVERT_DTR_PIN

}

void SIMGPRS::_releaseDtr()
{
	pinMode(_DTR_PIN, INPUT);
}

void SIMGPRS::_scanMessagesContent()
{
	isNewMessageChecked = true;
	LOGLNF("SCANING MESSAGES...");
	if (_msgCmdCounter == 0)
		return;
	String message = "", sender = "";
	if (!readMessage(&message, &sender, 1))
		return;

#ifdef IGNORE_SMS_UPPERCASE
	String lowercaseMessage = message;
	lowercaseMessage.toLowerCase();
	for (uint8_t i = 0; i < _msgCmdCounter; i++)
	{
		if (lowercaseMessage.indexOf(_msgCmdContainer[i].message) == 0)
		{
			_msgCmdContainer[i].function(&message, &sender);
			return;
		}
	}
#else
	for (uint8_t i = 0; i < _msgCmdCounter; i++)
	{
		if (message.indexOf(_msgCmdContainer[i].message) == 0)
		{
			_msgCmdContainer[i].function(&message, &sender);
			return;
		}
	}
#endif // IGNORE_SMS_UPPERCASE

	if (_defaultMessageFunction != nullptr)
		_defaultMessageFunction(&message, &sender); // This function runs when none of functions above is called
}

PowerStatus SIMGPRS::_sleepStatus()
{
	String resp;
	PowerStatus tempPowerStatus = _POWER_STATUS;
	_POWER_STATUS = PowerStatus::FULL_FUNC;
	if (waitForResponse_F(F("AT+CSCLK?"), DEFAULT_TRIES, DEFAULT_TIMEOUT, DEFAULT_RESPONSE_OK, DEFAULT_RESPONSE_ERROR, &resp))
	{
		_POWER_STATUS = tempPowerStatus;
		resp = resp.substring(resp.indexOf(":") + 1, resp.indexOf("OK") - 2);
		resp.trim();
		int status = resp.toInt();
		switch (status)
		{
		case 0:
			return PowerStatus::FULL_FUNC;
		case 1:
			return PowerStatus::SLEEP;
		case 2:
			return PowerStatus::AUTO_SLEEP;
		default:
			break;
		}
	}
	return PowerStatus::PRE_POWERD_OFF;
}

boolean SIMGPRS::_sendCmd_F(const __FlashStringHelper * cmd, uint8_t count)
{
	uint16_t i = 0;
	LOGF("CMD:   ");
	LOG(cmd);
	SEND(cmd);
	LOGF(" (");
	LOG(count);
	LOGLNF(")");
	if (_POWER_STATUS != PowerStatus::FULL_FUNC)
	{
		LOGLNF("\nERR: POWER STATUS IS NOT FULL FUNCTIONALITY\n");
		return false;
	}
	SEND(F("\r\n"));
	return true;
}

boolean SIMGPRS::_sendCmd_strptr(String * cmd, uint8_t count)
{
	LOGF("CMD:   ");
	LOG(*cmd);
	LOGF(" (");
	LOG(count);
	LOGLNF(")");
	if (_POWER_STATUS != PowerStatus::FULL_FUNC)
	{
		LOGLNF("\nERR: POWER STATUS IS NOT FULL FUNCTIONALITY\n");
		return false;
	}
	SEND(*cmd);
	SEND(F("\r\n"));
	return true;
}

boolean SIMGPRS::_sendCmd_str(String cmd, uint8_t count)
{
	LOGF("CMD:   ");
	LOG(cmd);
	LOGF(" (");
	LOG(count);
	LOGLNF(")");
	if (cmd != "AT+CSCLK=0" && _POWER_STATUS != PowerStatus::FULL_FUNC)
	{
		LOGLNF("\nERR: POWER STATUS IS NOT FULL FUNCTIONALITY\n");
		return false;
	}
	SEND(cmd);
	SEND(F("\r\n"));
	return true;
}

void SIMGPRS::_checkNewMessageNextTime()
{
	isNewMessageChecked = false;
}

boolean SIMGPRS::_waitForResponse(uint32_t * timeout, const __FlashStringHelper * okText, const __FlashStringHelper * errorText, String * responseText)
{
	uint32_t timerStart = millis();
	uint32_t execTime = 0;
	String tempSumString = "";
	String * tempPtr = &tempSumString;
	if (responseText != nullptr) {
		*responseText = "";
		tempPtr = responseText;
	}
	while (true) {
		while (AVAILABLE) {
			char c = READ;
#ifdef ENABLE_ECHO
			if (tempPtr->length() == 0)
				ECHOF("ECHO:  ");
			ECHO(c);
			if (c == '\n')
				ECHOF("       "); // Just to align the text
#endif // ENABLE_ECHO
			if (tempPtr->length() < MAX_RESPONSE_LENGTH)
				*tempPtr += c;
			if (tempPtr->length() == MAX_RESPONSE_LENGTH - 1) {
				LOGLNF("\nWRN: MAX RESPONSE SIZE REACHED");
				_clearBuffer();
			}
			SLOW_DOWN;
		}
		execTime = millis() - timerStart;
		// Catch fulfiled response
		if (tempPtr->indexOf(okText) >= 0) {
			LOGF("\n[");
			LOG(execTime);
			LOGF("ms]");
			LOGLNF("GOT EXPECTED STRING\n");
			return true;
		}
		// Catch ERROR
		if (tempPtr->indexOf(errorText) >= 0) {
			LOGF("\n[");
			LOG(execTime);
			LOGF("ms]");
			LOGLNF("ERR: ERROR CATCHED\n");
			return false;
		}
		// Catch timeout exception
		if (execTime > * timeout) {
			LOGF("\n[");
			LOG(execTime);
			LOGF("ms]");
			LOGLNF("ERR: TIMEOUT\n");
			return false;
		}
	}
	return false;
}

boolean _timediff(uint32_t * timer, uint32_t interval) {
	if (millis() > *timer) {
		if (millis() - *timer > interval) {
			*timer = millis();
			return true;
		}
		return false;
	}
	*timer = millis();
	return false;
}