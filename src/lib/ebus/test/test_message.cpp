/*
 * Copyright (C) John Baier 2014-2015 <ebusd@ebusd.eu>
 *
 * This file is part of ebusd.
 *
 * libebus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libebus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libebus. If not, see http://www.gnu.org/licenses/.
 */

#include "message.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

void verify(bool expectFailMatch, string type, string input,
		bool match, string expectStr, string gotStr)
{
	if (expectFailMatch) {
		if (match)
			cout << "  failed " << type << " match >" << input
			        << "< error: unexpectedly succeeded" << endl;
		else
			cout << "  failed " << type << " match >" << input << "< OK" << endl;
	}
	else if (match)
		cout << "  " << type << " match >" << input << "< OK" << endl;
	else
		cout << "  " << type << " match >" << input << "< error: got >"
		        << gotStr << "<, expected >" << expectStr << "<" << endl;
}

int main()
{
	// message:  [type],[circuit],name,[comment],[QQ[;QQ]*],[ZZ],[PBSB],[ID],fields...
	// field:    name,part,type[:len][,[divisor|values][,[unit][,[comment]]]]
	// template: name,type[:len][,[divisor|values][,[unit][,[comment]]]]
	// condition: name,circuit,messagename,[fieldname],values
	string checks[][5] = {
		// "message", "decoded", "master", "slave", "flags"
		{"date,HDA:3,,,Datum", "", "", "", "template"},
		{"time,VTI,,,", "", "", "", "template"},
		{"dcfstate,UCH,0=nosignal;1=ok;2=sync;3=valid,,", "", "", "", "template"},
		{"temp,D2C,,°C,Temperatur", "", "", "", "template"},
		{"temp2,D2B,,°C,Temperatur", "", "", "", "template"},
		{"power,UCH,,kW", "", "", "", "template"},
		{"sensor,UCH,0=ok;85=circuit;170=cutoff,,Fühlerstatus", "", "", "", "template"},
		{"tempsensor,temp;sensor,,Temperatursensor", "", "", "", "template"},
		{"r,message circuit,message name,message comment,,25,B509,0d2800,,,tempsensor", "temp=-14.00 Temperatursensor [Temperatur];sensor=ok [Fühlerstatus]", "ff25b509030d2800", "0320ff00", "mD"},
		{"r,message circuit,message name,message comment,,25,B509,0d2800,,,tempsensor,,field unit,field comment", "temp=-14.00 field unit [field comment];sensor=ok [Fühlerstatus]", "ff25b509030d2800", "0320ff00", "mD"},
		{"r,message circuit,message name,message comment,,25,B509,0d2800,,,temp,,field unit,field comment,,,sensor", "temp=-14.00 field unit [field comment];sensor=ok [Fühlerstatus]", "ff25b509030d2800", "0320ff00", "mD"},
		{"u,,first,,,fe,0700,,x,,bda", "26.10.2014", "fffe07000426100614", "00", "p"},
		{"u,broadcast,hwStatus,,,fe,b505,27,,,UCH,,,,,,UCH,,,,,,UCH,,,", "0;19;0", "10feb505042700130097", "00", ""},
		{"w,,first,,,15,b509,0400,date,,bda", "26.10.2014", "ff15b50906040026100614", "00", "m"},
		{"w,,first,,,15,b509", "", "ff15b50900", "00", "m"},
		{"r,ehp,time,,,08,b509,0d2800,,,time", "15:00:17", "ff08b509030d2800", "0311000f", "md"},
		{"r,ehp,time,,,08;10,b509,0d2800,,,time", "15:00:17", "ff08b509030d2800", "0311000f", "c"},
		{"r,ehp,time,,,08;09,b509,0d2800,,,time", "15:00:17", "ff08b509030d2800", "0311000f", "md*"},
		{"r,ehp,date,,,08,b509,0d2900,,,date", "23.11.2014", "ff08b509030d2900", "03170b0e", "md"},
		{"r,ehp,error,,,08,b509,0d2800,index,m,UCH,,,,,,time", "3;15:00:17", "ff08b509040d280003", "0311000f", "mdi"},
		{"r,ehp,error,,,08,b509,0d2800,index,m,UCH,,,,,,time", "index=3;time=15:00:17", "ff08b509040d280003", "0311000f", "mD"},
		{"u,ehp,ActualEnvironmentPower,Energiebezug,,08,B509,29BA00,,s,IGN:2,,,,,s,power", "8", "1008b5090329ba00", "03ba0008", "pm"},
		{"uw,ehp,test,Test,,08,B5de,ab,,,power,,,,,s,hex:1", "8;39", "1008b5de02ab08", "0139", "pm"},
		{"u,ehp,hwTankTemp,Speichertemperatur IST,,25,B509,290000,,,IGN:2,,,,,,tempsensor", "","","","M"},
		{"", "55.50;ok","1025b50903290000","050000780300","d"},
		{"r,ehp,datetime,Datum Uhrzeit,,50,B504,00,,,dcfstate,,,,time,,BTI,,,,date,,BDA,,,,temp,,temp2", "valid;08:24:51;31.12.2014;-0.875", "1050b5040100", "0a035124083112031420ff", "md" },
		{"r,ehp,bad,invalid pos,,50,B5ff,000102,,m,HEX:8;tempsensor;tempsensor;tempsensor;tempsensor;power;power,,,", "", "", "", "c" },
		{"r,ehp,bad,invalid pos,,50,B5ff,,,s,HEX:8;tempsensor;tempsensor;tempsensor;tempsensor;tempsensor;power;power,,,", "", "", "", "c" },
		{"r,ehp,ApplianceCode,,,08,b509,0d4301,,,UCH,", "9", "ff08b509030d4301", "0109", "d" },
		{"r,ehp,,,,08,b509,0d", "", "", "", "defaults" },
		{"[brinetowater],ehp,ApplianceCode,,4;6;8;9;10", "", "", "", "condition" },
		{"[airtowater]r,ehp,notavailable,,,,,0100,,,uch", "1", "ff08b509030d0100", "0101", "c" },
		{"[brinetowater]r,ehp,available,,,,,0100,,,uch", "1", "ff08b509030d0100", "0101", "d" },
	};
	DataFieldTemplates* templates = new DataFieldTemplates();
	MessageMap* messages = new MessageMap();
	vector< vector<string> > defaultsRows;
	map<string, Condition*> &conditions = messages->getConditions();
	Message* message = NULL;
	vector<Message*> deleteMessages;
	for (size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); i++) {
		string check[5] = checks[i];
		istringstream isstr(check[0]);
		string inputStr = check[1];
		SymbolString mstr(true);
		result_t result = mstr.parseHex(check[2]);
		if (result != RESULT_OK) {
			cout << "\"" << check[0] << "\": parse \"" << check[2] << "\" error: " << getResultCode(result) << endl;
			continue;
		}
		SymbolString sstr(true);
		result = sstr.parseHex(check[3]);
		if (result != RESULT_OK) {
			cout << "\"" << check[0] << "\": parse \"" << check[3] << "\" error: " << getResultCode(result) << endl;
			continue;
		}
		string flags = check[4];
		bool isTemplate = flags == "template";
		bool isCondition = flags == "condition";
		bool isDefaults = isCondition || flags == "defaults";
		bool dontMap = flags.find('m') != string::npos;
		bool onlyMap = flags.find('M') != string::npos;
		bool failedCreate = flags.find('c') != string::npos;
		bool decodeVerbose = flags.find('D') != string::npos;
		bool decode = decodeVerbose || (flags.find('d') != string::npos);
		bool failedPrepare = flags.find('p') != string::npos;
		bool failedPrepareMatch = flags.find('P') != string::npos;
		bool multi = flags.find('*') != string::npos;
		bool withInput = flags.find('i') != string::npos;
		string item;
		vector<string> entries;

		while (getline(isstr, item, FIELD_SEPARATOR) != 0)
			entries.push_back(item);

		if (deleteMessages.size()>0) {
			for (vector<Message*>::iterator it = deleteMessages.begin(); it != deleteMessages.end(); it++) {
				Message* deleteMessage = *it;
				delete deleteMessage;
			}
			deleteMessages.clear();
		}
		if (isTemplate) {
			// store new template
			DataField* fields = NULL;
			vector<string>::iterator it = entries.begin();
			result = DataField::create(it, entries.end(), templates, fields, false, true, false);
			if (result != RESULT_OK)
				cout << "\"" << check[0] << "\": template fields create error: " << getResultCode(result) << endl;
			else if (it != entries.end()) {
				cout << "\"" << check[0] << "\": template fields create error: trailing input " << static_cast<unsigned>(entries.end()-it) << endl;
			}
			else {
				cout << "\"" << check[0] << "\": create template OK" << endl;
				result = templates->add(fields, "", true);
				if (result == RESULT_OK)
					cout << "  store template OK" << endl;
				else {
					cout << "  store template error: " << getResultCode(result) << endl;
					delete fields;
				}
			}
			continue;
		}
		if (isDefaults) {
			// store defaults or condition
			vector<string>::iterator it = entries.begin();
			size_t oldSize = conditions.size();
			result = messages->addDefaultFromFile(defaultsRows, entries, it, "no file", 1);
			if (result != RESULT_OK)
				cout << "\"" << check[0] << "\": defaults read error: " << getResultCode(result) << endl;
			else if (it != entries.end())
				cout << "\"" << check[0] << "\": defaults read error: trailing input " << static_cast<unsigned>(entries.end()-it) << endl;
			else {
				cout << "\"" << check[0] << "\": read defaults OK" << endl;
				if (isCondition) {
					if (conditions.size()==oldSize) {
						cout << "  create condition error" << endl;
					} else {
						string error;
						result = messages->resolveConditions(error);
						if (result != RESULT_OK)
							cout << "  resolve conditions error: " << getResultCode(result) << " " << error << endl;
						else
							cout << "  resolve conditions OK" << endl;
					}
				}
			}
			continue;
		}
		if (entries.size() == 0) {
			message = messages->find(mstr);
			if (message == NULL) {
				cout << "\"" << check[2] << "\": find error: NULL" << endl;
				continue;
			}
			cout << "\"" << check[2] << "\": find OK" << endl;
		}
		else {
			vector<string>::iterator it = entries.begin();

			result = Message::create(it, entries.end(), &defaultsRows, &conditions, "no file", templates, deleteMessages);
			if (failedCreate) {
				if (result == RESULT_OK)
					cout << "\"" << check[0] << "\": failed create error: unexpectedly succeeded" << endl;
				else
					cout << "\"" << check[0] << "\": failed create OK" << endl;
				continue;
			}
			if (result != RESULT_OK) {
				cout << "\"" << check[0] << "\": create error: "
						<< getResultCode(result) << endl;
				printErrorPos(cout, entries.begin(), entries.end(), it, "", 0, result);
				continue;
			}
			if (deleteMessages.size()==0) {
				cout << "\"" << check[0] << "\": create error: NULL" << endl;
				continue;
			}
			if (it != entries.end()) {
				cout << "\"" << check[0] << "\": create error: trailing input " << static_cast<unsigned>(entries.end()-it) << endl;
				continue;
			}
			if (multi && deleteMessages.size()==1) {
				cout << "\"" << check[0] << "\": create error: single message instead of multiple" << endl;
				continue;
			}
			if (!multi && deleteMessages.size()>1) {
				cout << "\"" << check[0] << "\": create error: multiple messages instead of single" << endl;
				continue;
			}
			cout << "\"" << check[0] << "\": create OK" << endl;
			if (!dontMap) {
				result_t result = RESULT_OK;
				for (vector<Message*>::iterator it = deleteMessages.begin(); it != deleteMessages.end(); it++) {
					Message* deleteMessage = *it;
					result_t result = messages->add(deleteMessage);
					if (result != RESULT_OK) {
						cout << "\"" << check[0] << "\": add error: "
								<< getResultCode(result) << endl;
						break;
					}
				}
				if (result != RESULT_OK)
					continue;
				cout << "  map OK" << endl;
				message = deleteMessages.front();
				deleteMessages.clear();
				if (onlyMap)
					continue;
				Message* foundMessage = messages->find(mstr);
				if (foundMessage == message)
					cout << "  find OK" << endl;
				else if (foundMessage == NULL)
					cout << "  find error: NULL" << endl;
				else
					cout << "  find error: different" << endl;
			}
			else
				message = deleteMessages.front();
		}

		if (message->isPassive() || decode) {
			ostringstream output;
			result = message->decode(mstr, sstr, output, decodeVerbose?OF_VERBOSE:0);
			if (result != RESULT_OK) {
				cout << "  \"" << check[2] << "\" / \"" << check[3] << "\": decode error: "
						<< getResultCode(result) << endl;
				continue;
			}
			cout << "  \"" << check[2] << "\" / \"" << check[3] <<  "\": decode OK" << endl;
			bool match = inputStr == output.str();
			verify(false, "decode", check[2] + "/" + check[3], match, inputStr, output.str());
			ostringstream output2;
			result = message->decodeLastData(output2, decodeVerbose?OF_VERBOSE:0);
			if (result != RESULT_OK) {
				cout << "  \"" << check[2] << "\" / \"" << check[3] << "\": decodeLast error: "
						<< getResultCode(result) << endl;
				continue;
			}
			cout << "  \"" << check[2] << "\" / \"" << check[3] <<  "\": decodeLast OK" << endl;
			match = output.str() == output2.str();
			verify(false, "decodeLast", check[2] + "/" + check[3], match, output.str(), output2.str());
		}
		if (!message->isPassive() && (withInput || !decode)) {
			istringstream input(inputStr);
			SymbolString writeMstr;
			result = message->prepareMaster(0xff, writeMstr, input);
			if (failedPrepare) {
				if (result == RESULT_OK)
					cout << "  \"" << inputStr << "\": failed prepare error: unexpectedly succeeded" << endl;
				else
					cout << "  \"" << inputStr << "\": failed prepare OK" << endl;
				continue;
			}

			if (result != RESULT_OK) {
				cout << "  \"" << inputStr << "\": prepare error: "
						<< getResultCode(result) << endl;
				continue;
			}
			cout << "  \"" << inputStr << "\": prepare OK" << endl;

			bool match = writeMstr==mstr;
			verify(failedPrepareMatch, "prepare", inputStr, match, mstr.getDataStr(), writeMstr.getDataStr());
		}
	}

	if (deleteMessages.size()>0) {
		for (vector<Message*>::iterator it = deleteMessages.begin(); it != deleteMessages.end(); it++) {
			Message* deleteMessage = *it;
			delete deleteMessage;
		}
		deleteMessages.clear();
	}

	delete templates;
	delete messages;

	return 0;

}
