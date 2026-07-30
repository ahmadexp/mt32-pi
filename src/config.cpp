//
// config.cpp
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2020-2023 Dale Whinham <daleyo@gmail.com>
//
// This file is part of mt32-pi.
//
// mt32-pi is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// mt32-pi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// mt32-pi. If not, see <http://www.gnu.org/licenses/>.
//

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>

#include <circle/logger.h>
#include <circle/util.h>
#include <fatfs/ff.h>
#include <ini.h>

#include "config.h"
#include "utility.h"

LOGMODULE("config");
const char* TrueStrings[]  = {"true", "on", "1"};
const char* FalseStrings[] = {"false", "off", "0"};
constexpr size_t MaxConfigFileSize = 64 * 1024;

// Templated function that converts a string to an enum
template <class T, const char* pEnumStrings[], size_t N> static bool ParseEnum(const char* pString, T* pOut)
{
	for (size_t i = 0; i < N; ++i)
	{
		if (!strcasecmp(pString, pEnumStrings[i]))
		{
			*pOut = static_cast<T>(i);
			return true;
		}
	}

	return false;
}

// Macro to expand templated enum parser into an overloaded definition of ParseOption()
#define CONFIG_ENUM_PARSER(ENUM_NAME)                                                                           \
	bool CConfig::ParseOption(const char* pString, ENUM_NAME* pOut)                                             \
	{                                                                                                           \
		return ParseEnum<ENUM_NAME, ENUM_NAME##Strings, Utility::ArraySize(ENUM_NAME##Strings)>(pString, pOut); \
	}

// Enum string tables
CONFIG_ENUM_STRINGS(TSystemDefaultSynth, ENUM_SYSTEMDEFAULTSYNTH);
CONFIG_ENUM_STRINGS(TAudioOutputDevice, ENUM_AUDIOOUTPUTDEVICE);
CONFIG_ENUM_STRINGS(TMT32EmuResamplerQuality, ENUM_RESAMPLERQUALITY);
CONFIG_ENUM_STRINGS(TMT32EmuMIDIChannels, ENUM_MIDICHANNELS);
CONFIG_ENUM_STRINGS(TMT32EmuROMSet, ENUM_MT32ROMSET);
CONFIG_ENUM_STRINGS(TLCDType, ENUM_LCDTYPE);
CONFIG_ENUM_STRINGS(TControlScheme, ENUM_CONTROLSCHEME);
CONFIG_ENUM_STRINGS(TEncoderType, ENUM_ENCODERTYPE);
CONFIG_ENUM_STRINGS(TLCDRotation, ENUM_LCDROTATION);
CONFIG_ENUM_STRINGS(TLCDMirror, ENUM_LCDMIRROR);
CONFIG_ENUM_STRINGS(TNetworkMode, ENUM_NETWORKMODE);

CConfig* CConfig::s_pThis = nullptr;

CConfig::CConfig()
{
	// Expand assignment of all default values from definition file
	#define CFG(_1, _2, MEMBER_NAME, DEFAULT, _3...) MEMBER_NAME = DEFAULT;
	#include "config.def"

	s_pThis = this;
}

bool CConfig::Initialize(const char* pPath)
{
	FIL File;
	if (f_open(&File, pPath, FA_READ) != FR_OK)
	{
		LOGERR("Couldn't open '%s' for reading", pPath);
		return false;
	}

	const FSIZE_t nFileSize = f_size(&File);
	if (nFileSize > MaxConfigFileSize)
	{
		LOGERR("Config file '%s' is too large", pPath);
		f_close(&File);
		return false;
	}

	// Keep user-controlled file sizes off the core stack.
	const UINT nSize = static_cast<UINT>(nFileSize);
	std::unique_ptr<char[]> Buffer(new char[nSize + 1]);
	if (!Buffer)
	{
		LOGERR("Not enough memory to read config file '%s'", pPath);
		f_close(&File);
		return false;
	}

	UINT nRead;

	if (f_read(&File, Buffer.get(), nSize, &nRead) != FR_OK || nRead != nSize)
	{
		LOGERR("Error reading config file '%s'", pPath);
		f_close(&File);
		return false;
	}

	// Ensure null-terminated
	Buffer[nRead] = '\0';
	f_close(&File);

	const int nResult = ini_parse_string(Buffer.get(), INIHandler, this);
	if (nResult > 0)
		LOGWARN("Config parse error on line %d", nResult);

	return nResult == 0 && Validate();
}

int CConfig::INIHandler(void* pUser, const char* pSection, const char* pName, const char* pValue)
{
	CConfig* const pConfig = static_cast<CConfig*>(pUser);

	//LOGDBG("'%s', '%s', '%s'", pSection, pName,  pValue);

	// Automatically generate ParseOption() calls from macro definition file
	#define BEGIN_SECTION(SECTION)       \
		if (!strcmp(#SECTION, pSection)) \
		{

	#define CFG(INI_NAME, TYPE, MEMBER_NAME, _2, ...) \
		if (!strcmp(#INI_NAME, pName))                \
			return ParseOption(pValue, &pConfig->MEMBER_NAME __VA_OPT__(, ) __VA_ARGS__);

	#define END_SECTION \
		return 0;       \
		}

	#include "config.def"

	return 0;
}

bool CConfig::ParseOption(const char* pString, bool* pOutBool)
{
	for (auto pTrueString : TrueStrings)
	{
		if (!strcasecmp(pString, pTrueString))
		{
			*pOutBool = true;
			return true;
		}
	}

	for (auto pFalseString : FalseStrings)
	{
		if (!strcasecmp(pString, pFalseString))
		{
			*pOutBool = false;
			return true;
		}
	}

	return false;
}

bool CConfig::ParseOption(const char* pString, int* pOutInt, bool bHex)
{
	errno = 0;
	char* pEnd;
	const long nValue = strtol(pString, &pEnd, bHex ? 16 : 10);
	if (pEnd == pString || *pEnd != '\0' || errno == ERANGE ||
	    nValue < std::numeric_limits<int>::min() || nValue > std::numeric_limits<int>::max())
		return false;

	*pOutInt = static_cast<int>(nValue);
	return true;
}

bool CConfig::ParseOption(const char* pString, float* pOutFloat)
{
	errno = 0;
	char* pEnd;
	const float nValue = strtof(pString, &pEnd);
	if (pEnd == pString || *pEnd != '\0' || errno == ERANGE || !std::isfinite(nValue))
		return false;

	*pOutFloat = nValue;
	return true;
}

bool CConfig::ParseOption(const char* pString, CString* pOut)
{
	*pOut = CString(pString);
	return true;
}

bool CConfig::ParseOption(const char* pString, CIPAddress* pOut)
{
	// Space for 4 period-separated groups of 3 digits plus null terminator
	char Buffer[16];
	u8 IPAddress[4];

	const size_t nLength = strlen(pString);
	if (nLength >= sizeof(Buffer))
		return false;

	memcpy(Buffer, pString, nLength + 1);
	char* pToken = strtok(Buffer, ".");

	for (uint8_t i = 0; i < 4; ++i)
	{
		if (!pToken)
			return false;

		char* pEnd;
		const unsigned long nOctet = strtoul(pToken, &pEnd, 10);
		if (pEnd == pToken || *pEnd || nOctet > 255)
			return false;

		IPAddress[i] = static_cast<u8>(nOctet);
		pToken = strtok(nullptr, ".");
	}
	if (pToken)
		return false;

	pOut->Set(IPAddress);
	return true;
}

bool CConfig::Validate() const
{
	#define VALIDATE_RANGE(NAME, MINIMUM, MAXIMUM) \
		if (NAME < (MINIMUM) || NAME > (MAXIMUM)) \
		{ \
			LOGERR("Config value '%s' is out of range [%d, %d]", #NAME, MINIMUM, MAXIMUM); \
			return false; \
		}

	VALIDATE_RANGE(SystemI2CBaudRate, 100000, 1000000);
	VALIDATE_RANGE(SystemPowerSaveTimeout, 0, 3600);
	VALIDATE_RANGE(MIDIGPIOBaudRate, 300, 4000000);
	VALIDATE_RANGE(MIDIUSBSerialBaudRate, 9600, 115200);
	VALIDATE_RANGE(AudioSampleRate, 32000, 192000);
	VALIDATE_RANGE(AudioChunkSize, 2, 2048);
	VALIDATE_RANGE(ControlSwitchTimeout, 0, 3600);
	VALIDATE_RANGE(FluidSynthSoundFont, 0, 255);
	VALIDATE_RANGE(FluidSynthPolyphony, 1, 65535);
	VALIDATE_RANGE(LCDI2CLCDAddress, 0, 0x7F);

	#undef VALIDATE_RANGE

	if ((LCDType == TLCDType::HD44780FourBit || LCDType == TLCDType::HD44780I2C) &&
	    !((LCDWidth == 16 || LCDWidth == 20) && (LCDHeight == 2 || LCDHeight == 4)))
	{
		LOGERR("Character LCD dimensions must be 16x2, 16x4, 20x2, or 20x4");
		return false;
	}

	if ((LCDType == TLCDType::SH1106I2C || LCDType == TLCDType::SSD1306I2C) &&
	    !((LCDWidth == 128 || LCDWidth == 132) && (LCDHeight == 32 || LCDHeight == 64)))
	{
		LOGERR("Graphical LCD dimensions must be 128x32, 128x64, 132x32, or 132x64");
		return false;
	}

	if (AudioOutputDevice == TAudioOutputDevice::I2S && AudioChunkSize < 32)
	{
		LOGERR("I2S audio requires chunk_size >= 32");
		return false;
	}

	if (AudioOutputDevice == TAudioOutputDevice::HDMI && AudioChunkSize < 384)
	{
		LOGERR("HDMI audio requires chunk_size >= 384");
		return false;
	}

	return true;
}

// Define template function wrappers for parsing enums
CONFIG_ENUM_PARSER(TSystemDefaultSynth);
CONFIG_ENUM_PARSER(TAudioOutputDevice);
CONFIG_ENUM_PARSER(TMT32EmuResamplerQuality);
CONFIG_ENUM_PARSER(TMT32EmuMIDIChannels);
CONFIG_ENUM_PARSER(TMT32EmuROMSet);
CONFIG_ENUM_PARSER(TLCDType);
CONFIG_ENUM_PARSER(TControlScheme);
CONFIG_ENUM_PARSER(TEncoderType);
CONFIG_ENUM_PARSER(TLCDRotation);
CONFIG_ENUM_PARSER(TLCDMirror);
CONFIG_ENUM_PARSER(TNetworkMode);
