/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_ADC_COMMAND_H
#define DCPLUSPLUS_DCPP_ADC_COMMAND_H

#include <airdcpp/core/header/typedefs.h>

#include <airdcpp/core/classes/Exception.h>

namespace dcpp {

class CID;

class AdcCommand {
public:
	template<uint32_t T>
	struct Type {
		enum { CMD = T };
	};

	using ParamMap = multimap<string, string>;
	using ParamList = StringList;

	enum Error {
		SUCCESS = 0,
		ERROR_GENERIC = 0,
		ERROR_HUB_GENERIC = 10,
		ERROR_HUB_FULL = 11,
		ERROR_HUB_DISABLED = 12,
		ERROR_LOGIN_GENERIC = 20,
		ERROR_NICK_INVALID = 21,
		ERROR_NICK_TAKEN = 22,
		ERROR_BAD_PASSWORD = 23,
		ERROR_CID_TAKEN = 24,
		ERROR_COMMAND_ACCESS = 25,
		ERROR_REGGED_ONLY = 26,
		ERROR_INVALID_PID = 27,
		ERROR_BANNED_GENERIC = 30,
		ERROR_PERM_BANNED = 31,
		ERROR_TEMP_BANNED = 32,
		ERROR_PROTOCOL_GENERIC = 40,
		ERROR_PROTOCOL_UNSUPPORTED = 41,
		ERROR_CONNECT_FAILED = 42,
		ERROR_INF_MISSING = 43,
		ERROR_BAD_STATE = 44,
		ERROR_FEATURE_MISSING = 45,
		ERROR_BAD_IP = 46,
		ERROR_NO_HUB_HASH = 47,
		ERROR_TRANSFER_GENERIC = 50,
		ERROR_FILE_NOT_AVAILABLE = 51,
		ERROR_FILE_PART_NOT_AVAILABLE = 52,
		ERROR_SLOTS_FULL = 53,
		ERROR_NO_CLIENT_HASH = 54,
		ERROR_HBRI_TIMEOUT = 55,
		ERROR_FILE_ACCESS_DENIED = 60,
		ERROR_UNKNOWN_USER = 61,
		ERROR_TLS_REQUIRED = 62
	};

	enum Severity {
		SEV_SUCCESS = 0,
		SEV_RECOVERABLE = 1,
		SEV_FATAL = 2
	};

	static const char TYPE_BROADCAST = 'B';
	static const char TYPE_CLIENT = 'C';
	static const char TYPE_DIRECT = 'D';
	static const char TYPE_ECHO = 'E';
	static const char TYPE_FEATURE = 'F';
	static const char TYPE_INFO = 'I';
	static const char TYPE_HUB = 'H';
	static const char TYPE_UDP = 'U';

#define ADC_CMD(n, a, b, c) static const uint32_t CMD_##n = (((uint32_t)a) | (((uint32_t)b)<<8) | (((uint32_t)c)<<16)); using n = AdcCommand::Type<CMD_##n>
	// Base commands
	ADC_CMD(SUP, 'S','U','P');
	ADC_CMD(STA, 'S','T','A');
	ADC_CMD(INF, 'I','N','F');
	ADC_CMD(MSG, 'M','S','G');
	ADC_CMD(SCH, 'S','C','H');
	ADC_CMD(RES, 'R','E','S');
	ADC_CMD(CTM, 'C','T','M');
	ADC_CMD(RCM, 'R','C','M');
	ADC_CMD(GPA, 'G','P','A');
	ADC_CMD(PAS, 'P','A','S');
	ADC_CMD(QUI, 'Q','U','I');
	ADC_CMD(GET, 'G','E','T');
	ADC_CMD(GFI, 'G','F','I');
	ADC_CMD(SND, 'S','N','D');
	ADC_CMD(SID, 'S','I','D');

	// Extensions
	ADC_CMD(CMD, 'C','M','D');

	ADC_CMD(NAT, 'N','A','T');
	ADC_CMD(RNT, 'R','N','T');

	ADC_CMD(ZON, 'Z','O','N');
	ADC_CMD(ZOF, 'Z','O','F');

	ADC_CMD(TCP, 'T','C','P');

	ADC_CMD(PMI, 'P', 'M', 'I');

	using CommandType = uint32_t;

	static const dcpp::SID HUB_SID = 0xffffffff;		// No client will have this sid

	static uint32_t toFourCC(const char* x) noexcept { return *reinterpret_cast<const uint32_t*>(x); }
	static std::string fromFourCC(uint32_t x) noexcept { return std::string(reinterpret_cast<const char*>(&x), sizeof(x)); }

	explicit AdcCommand(uint32_t aCmd, char aType = TYPE_CLIENT) noexcept;
	explicit AdcCommand(uint32_t aCmd, dcpp::SID aTarget, char aType) noexcept;
	explicit AdcCommand(Severity sev, Error err, const string& desc, char aType = TYPE_CLIENT) noexcept;

	// Throws ParseException on errors
	explicit AdcCommand(const string& aLine, bool nmdc = false);

	// Throws ParseException on errors
	void parse(const string& aLine, bool nmdc = false);

	uint32_t getCommand() const noexcept { return cmdInt; }
	char getType() const noexcept { return type; }
	void setType(char t) noexcept { type = t; }
	string getFourCC() const noexcept { string tmp(4, 0); tmp[0] = type; tmp[1] = cmd[0]; tmp[2] = cmd[1]; tmp[3] = cmd[2]; return tmp; }

	const string& getFeatures() const noexcept { return features; }
	AdcCommand& setFeatures(const string& feat) noexcept { features = feat; return *this; }

	enum class FeatureType {
		REQUIRED,
		EXCLUDED,
	};
	AdcCommand& addFeature(const string& feat, FeatureType aType) noexcept;

	ParamList& getParameters() noexcept { return parameters; }
	const ParamList& getParameters() const noexcept { return parameters; }
	AdcCommand& setParams(const ParamList& aParams) noexcept {
		parameters = aParams; 
		return *this; 
	}

	string toString() const noexcept;
	string toString(const CID& aCID) const noexcept;
	string toString(dcpp::SID sid, bool nmdc = false) const noexcept;

	AdcCommand& addParam(const string& name, const string& value) noexcept {
		parameters.push_back(name);
		parameters.back() += value;
		return *this;
	}
	AdcCommand& addParam(const string& str) noexcept {
		parameters.push_back(str);
		return *this;
	}
	AdcCommand& addParams(const ParamMap& aParams) noexcept;
	const string& getParam(size_t n) const noexcept;
	/** Return a named parameter where the name is a two-letter code */
	bool getParam(const char* name, size_t start, string& ret) const noexcept;
	bool getParam(const char* name, size_t start, StringList& ret) const noexcept;
	bool hasFlag(const char* name, size_t start) const noexcept;
	static uint16_t toCode(const char* x) noexcept { return *((uint16_t*)x); }

	static CommandType toCommand(const string& aCmd) noexcept;
	static string fromCommand(CommandType x) noexcept;

	bool operator==(uint32_t aCmd) const noexcept { return cmdInt == aCmd; }

	static string escape(const string& str, bool old) noexcept;
	dcpp::SID getTo() const noexcept { return to; }
	AdcCommand& setTo(const dcpp::SID sid) noexcept { to = sid; return *this; }
	dcpp::SID getFrom() const noexcept { return from; }
	void setFrom(const dcpp::SID sid) noexcept { from = sid; }
	static bool isValidType(char aType) noexcept;

	static dcpp::SID toSID(const string& aSID) noexcept { return *reinterpret_cast<const dcpp::SID*>(aSID.data()); }
	static string fromSID(dcpp::SID aSID) noexcept { return string(reinterpret_cast<const char*>(&aSID), sizeof(aSID)); }
private:
	string getHeaderString(const CID& cid) const noexcept;
	string getHeaderString() const noexcept;
	string getHeaderString(dcpp::SID sid, bool nmdc) const noexcept;
	string getParamString(bool nmdc) const noexcept;
	ParamList parameters;
	string features;
	union {
		char cmdChar[4];
		uint8_t cmd[4];
		uint32_t cmdInt;
	};
	dcpp::SID from = 0;
	dcpp::SID to = 0;
	char type;

};

template<class T>
class CommandHandler {
public:
	using OnCommandParsedF = std::function<void (const AdcCommand &)>;
	inline void dispatch(const string& aLine, OnCommandParsedF&& aOnCommandParsedF) noexcept {
		dispatch(aLine, false, std::move(aOnCommandParsedF));
	}

	template<typename... ArgT>
	void dispatch(const string& aLine, bool aNmdc, const OnCommandParsedF& aOnCommandParsedF, ArgT&&... args) noexcept {
		try {
			AdcCommand c(aLine, aNmdc);
			if (!aNmdc && aOnCommandParsedF) {
				aOnCommandParsedF(c);
			}

			dispatch(c, std::forward<ArgT>(args)...);
		} catch (const ParseException&) {
			dcdebug("Invalid ADC command: %.50s\n", aLine.c_str());
			return;
		}
	}

	template<typename... ArgT>
	void dispatch(AdcCommand& aCmd, ArgT&&... args) noexcept {
#define C(n) case AdcCommand::CMD_##n: ((T*)this)->handle(AdcCommand::n(), aCmd, std::forward<ArgT>(args)...); break
		switch(aCmd.getCommand()) {
			C(SUP);
			C(STA);
			C(INF);
			C(MSG);
			C(SCH);
			C(RES);
			C(CTM);
			C(RCM);
			C(GPA);
			C(PAS);
			C(QUI);
			C(GET);
			C(GFI);
			C(SND);
			C(SID);
			C(CMD);
			C(NAT);
			C(RNT);
			C(ZON);
			C(ZOF);
			C(TCP);
			C(PMI);
		default: 
			// dcdebug("Unknown ADC command: %.50s\n", aCmd.toString().c_str());
			break;
#undef C
		}
	}
};

} // namespace dcpp

#endif // !defined(ADC_COMMAND_H)