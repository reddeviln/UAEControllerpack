#pragma once
#include "pch.h"
#include "UAEControllerpack2.h"
#include "loguru.cpp"

#define MY_PLUGIN_NAME      "Controller Pack UAEvACC"
#define MY_PLUGIN_VERSION   "1.5.4"
#define MY_PLUGIN_DEVELOPER "Nils Dornbusch"
#define MY_PLUGIN_COPYRIGHT "Licensed under GNU GPLv3"
#define MY_PLUGIN_VIEW      ""

//Tag items
const int TAG_ITEM_CTOT = 1121312;
const int TAG_ITEM_Sequence = 212312;
const int TAG_ITEM_TOBT = 312312;
const int TAG_ITEM_ROUTE_VALID = 123123;
const int TAG_ITEM_RECAT = 155;
const int TAG_ITEM_RECAT_NOSLASH = 12341;
const int TAG_ITEM_STAND = 1548915;

//Functions
const int TAG_FUNC_EDIT = 423;
const int TAG_FUNC_ASSIGN_POPUP = 456456;
const int TAG_FUNC_ASSIGN_AUTO = 412;
const int TAG_FUNC_ASSIGN_CARGO = 4578;
const int TAG_FUNC_ASSIGN_PAX = 456;
const int TAG_FUNC_ASSIGN_UAE = 31854;
const int TAG_FUNC_ASSIGN_LOWCOST = 3458;
const int TAG_FUNC_ASSIGN_VIP = 4868;
const int TAG_FUNC_ASSIGN_GA = 486;
const int TAG_FUNC_ASSIGN_ABY = 2342;
const int TAG_FUNC_ASSIGN_CARGO2 = 23230;
const int TAG_FUNC_MANUAL_FINISH = 2345;
const int TAG_FUNC_ASSIGN_ETD = 568978;
const int TAG_FUNC_CLEAR = 264;
const int TAG_FUNC_ROUTING = 15;
const int TAG_FUNC_ROUTING_OPT = 8463682;
const int TAG_FUNC_CTOT_MANUAL = 1;
const int TAG_FUNC_CTOT_ASSIGN = 2;
const int TAG_FUNC_CTOT_MANUAL_FINISH = 10;
const int TAG_FUNC_CTOT_ASSIGN_SEQ = 11;
const int TAG_FUNC_CTOT_ASSIGN_ASAP = 13;
const int TAG_FUNC_CTOT_CLEAR = 86715123;

//Global variables
const CTimeSpan taxitime = CTimeSpan(0, 0, 20, 0);
const double TOL = 0.02;
std::unordered_map<std::string, std::string> recatdict;
std::unordered_map<std::string, std::string> parkingdict;
std::string directory;
std::unordered_map<std::string, std::unordered_map<std::string, Stand>> data;
std::unordered_map<std::string, std::unordered_map<std::string, Stand>> standmapping;
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> callsignmap;
std::unordered_map<std::string, std::vector<Stand>> standsUAE;
std::unordered_map<std::string, std::vector<Stand>> standsPAX;
std::unordered_map<std::string, std::vector<Stand>> standsCARGO;
std::unordered_map<std::string, std::vector<Stand>> standsLOWCOST;
std::unordered_map<std::string, std::vector<Stand>> standsVIP;
std::unordered_map<std::string, std::vector<Stand>> standsGA;
std::unordered_map<std::string, std::vector<Stand>> standsOverflow;
std::unordered_map<std::string, std::vector<Stand>> standsABY;
std::unordered_map<std::string, std::vector<Stand>> standsETD;
std::unordered_map<std::string, std::vector<Stand>> standsCargoSpecial;
std::vector<std::string> activeAirports;
std::unordered_map<std::string, RouteData> routedataoptional;
std::unordered_map<std::string, RouteData> routedatamandatory;
//Constructor (run at plugin load)
CUAEController::CUAEController(void)
	: CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE,
		MY_PLUGIN_NAME,
		MY_PLUGIN_VERSION,
		MY_PLUGIN_DEVELOPER,
		MY_PLUGIN_COPYRIGHT)
{
	char path[MAX_PATH];
	HMODULE hm = NULL;
	//workaround to get dll path
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&CUAEController::test, &hm) == 0)
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}
	if (GetModuleFileName(hm, path, sizeof(path)) == 0)
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}
	std::string dir(path);
	std::string filename("UAEControllerpack.dll");
	size_t pos = dir.find(filename);
	dir.replace(pos, filename.length(), "");
	loguru::set_thread_name("UAEControllerpack");
	//putting a logfile in place
	std::string temp = dir + "UAEC.log";
	loguru::add_file(temp.c_str(), loguru::Truncate, loguru::Verbosity_INFO);
	std::string logstring = "We successfully started UAEControllerPack version ";
	logstring += MY_PLUGIN_VERSION;
	logstring += ". Great success!";
	LOG_F(INFO, logstring.c_str());
	
	//registering all Functions and Tag items
	RegisterTagItemType("RECAT", TAG_ITEM_RECAT);
	RegisterTagItemType("RECATnoSlash", TAG_ITEM_RECAT_NOSLASH);
	RegisterTagItemType("Stand", TAG_ITEM_STAND);
	RegisterTagItemFunction("Edit Stand", TAG_FUNC_EDIT);
	RegisterTagItemFunction("Assign Stand", TAG_FUNC_ASSIGN_POPUP);
	RegisterTagItemFunction("Clear", TAG_FUNC_CLEAR);
	RegisterTagItemType("RouteValid", TAG_ITEM_ROUTE_VALID);
	RegisterTagItemFunction("Get routes (mandatory)", TAG_FUNC_ROUTING);
	RegisterTagItemFunction("Get routes (optional)", TAG_FUNC_ROUTING_OPT);
	RegisterTagItemType("CTOT", TAG_ITEM_CTOT);
	RegisterTagItemType("T/O sequence", TAG_ITEM_Sequence);
	RegisterTagItemType("TOBT", TAG_ITEM_TOBT);
	RegisterTagItemFunction("Assign CTOT", TAG_FUNC_CTOT_ASSIGN);
	RegisterTagItemFunction("Edit CTOT", TAG_FUNC_CTOT_MANUAL);
	m_TOSequenceList = RegisterFpList("T/O Sequence List");
	if (m_TOSequenceList.GetColumnNumber() == 0)
	{
		// fill in the default columns of the list in none are available from settings
		m_TOSequenceList.AddColumnDefinition("C/S", 10, true, NULL, EuroScopePlugIn::TAG_ITEM_TYPE_CALLSIGN,
			NULL, EuroScopePlugIn::TAG_ITEM_FUNCTION_NO,
			NULL, EuroScopePlugIn::TAG_ITEM_FUNCTION_NO);
		m_TOSequenceList.AddColumnDefinition("CTOT", 4, true, MY_PLUGIN_NAME, TAG_ITEM_CTOT,
			NULL, EuroScopePlugIn::TAG_ITEM_FUNCTION_NO,
			NULL, EuroScopePlugIn::TAG_ITEM_FUNCTION_NO);
		m_TOSequenceList.AddColumnDefinition("#", 2, true, MY_PLUGIN_NAME, TAG_ITEM_Sequence,
			NULL, EuroScopePlugIn::TAG_ITEM_FUNCTION_NO,
			NULL, EuroScopePlugIn::TAG_ITEM_FUNCTION_NO);
		m_TOSequenceList.AddColumnDefinition("W", 1, true, NULL, EuroScopePlugIn::TAG_ITEM_TYPE_AIRCRAFT_CATEGORY,
			NULL, EuroScopePlugIn::TAG_ITEM_FUNCTION_NO,
			NULL, EuroScopePlugIn::TAG_ITEM_FUNCTION_NO);

	}

	directory = dir;
	LOG_F(INFO, "Everything registered. Ready to go!");

	//----------------------------------------Loading of all the different files--------------------------------------------

	//1. RECAT dictionary
	dir += "RECAT.csv";
	io::CSVReader<3, io::trim_chars<' '>, io::no_quote_escape<','>> in(dir);
	in.read_header(io::ignore_extra_column, "Aircraft", "RECAT", "PARKING");
	std::string aircraft, recat, parkingcode;
	while (in.read_row(aircraft, recat, parkingcode))
	{
		std::pair<std::string, std::string> temp(aircraft, recat);
		std::pair<std::string, std::string> tmp(aircraft, parkingcode);
		recatdict.insert(temp);
		parkingdict.insert(tmp);
	}
	LOG_F(INFO, "RECAT dictionary read without issues!");
	///////////////////////////////////////////////////////////////////////////////////
	//-------------------------------------------2. All Airport stands and mappings
	////////////////////////////////////////////////////////////////////////////////////
	std::regex icao(R"(.*\\([A-Z]{4}))");
	for (auto entry : std::filesystem::directory_iterator(directory))
	{
		if (entry.is_directory())
		{
			std::smatch m;
			std::string name = entry.path().string();
			if (std::regex_search(name, m, icao))
			{
				std::string temp = "/";
				auto temp2 = temp;
				temp += m.str(1);
				temp += ".csv";
				temp2 += "CallsignMap";
				temp2 += m.str(1);
				temp2 += ".csv";
				auto standpath = std::filesystem::path(name + temp);
				auto callsignpath = std::filesystem::path(name + temp2);
				if (std::filesystem::exists(standpath) && std::filesystem::exists(callsignpath))
				{
					readStandFile(standpath.string(), m.str(1));
					readCallsignFile(callsignpath.string(), m.str(1));
					activeAirports.push_back(m.str(1));
					std::string logstring;
					logstring = "Successfully parsed stand data for ";
					logstring += m[0];
					logstring += ".";
					LOG_F(INFO, logstring.c_str());
				}
				else
				{
					std::string logstring;
					logstring = "Not all required csv files found for ";
					logstring += m[0];
					logstring += ". Skipping this airport ...";
					LOG_F(INFO, logstring.c_str());
				}
			}
		}
	}
	//data handling of parsed stands
	for (auto airport : activeAirports)
	{
		auto found = data.find(airport);
		if (found == data.end()) break;
		std::vector<Stand> thisstandsUAE;
		std::vector<Stand> thisstandsABY;
		std::vector<Stand> thisstandsPAX;
		std::vector<Stand> thisstandsCARGO;
		std::vector<Stand> thisstandsCARGOspec;
		std::vector<Stand> thisstandsLOWCOST;
		std::vector<Stand> thisstandsGA;
		std::vector<Stand> thisstandsETD;
		std::vector<Stand> thisstandsVIP;
		std::vector<Stand> thisstandsOverflow;
		for (auto stand : found->second)
		{
			auto code = stand.second.mAirlinecode;
			if (code == "UAE")
			{
				thisstandsUAE.push_back(stand.second);
				continue;
			}
			if (code == "ABY")
			{
				thisstandsABY.push_back(stand.second);
				continue;
			}
			if (code == "PAX")
			{
				thisstandsPAX.push_back(stand.second);
				continue;
			}
			if (code == "ETD")
			{
				thisstandsETD.push_back(stand.second);
				continue;
			}

			if (code == "CARGO" || code == "CLC")
				thisstandsCARGO.push_back(stand.second);
			if (code == "LWC" || code == "CLC")
				thisstandsLOWCOST.push_back(stand.second);
			if (code == "GA")
			{
				thisstandsGA.push_back(stand.second);
				continue;
			}
			if (code == "CARGO1")
			{
				thisstandsCARGOspec.push_back(stand.second);
				continue;
			}
			if (code == "VIP")
			{
				thisstandsVIP.push_back(stand.second);
				continue;
			}
			if (code == "ALL")
			{
				thisstandsOverflow.push_back(stand.second);
				continue;
			}


		}
		std::pair<std::string, std::vector<Stand>> temp(airport, thisstandsUAE);
		standsUAE.insert(temp);
		std::pair<std::string, std::vector<Stand>> temp2(airport, thisstandsABY);
		standsABY.insert(temp2);
		std::pair<std::string, std::vector<Stand>> temp3(airport, thisstandsPAX);
		standsPAX.insert(temp3);
		std::pair<std::string, std::vector<Stand>> temp4(airport, thisstandsCARGO);
		standsCARGO.insert(temp4);
		std::pair<std::string, std::vector<Stand>> temp5(airport, thisstandsCARGOspec);
		standsCargoSpecial.insert(temp5);
		std::pair<std::string, std::vector<Stand>> temp6(airport, thisstandsLOWCOST);
		standsLOWCOST.insert(temp6);
		std::pair<std::string, std::vector<Stand>> temp7(airport, thisstandsGA);
		standsGA.insert(temp7);
		std::pair<std::string, std::vector<Stand>> temp8(airport, thisstandsVIP);
		standsVIP.insert(temp8);
		std::pair<std::string, std::vector<Stand>> temp9(airport, thisstandsOverflow);
		standsOverflow.insert(temp9);
		std::pair<std::string, std::vector<Stand>> temp10(airport, thisstandsETD);
		standsETD.insert(temp10);

	}
	/////////////////////////////////////////////////////////////////////////////////////
    //---------------------------3. Route restriction files (mandatory and optional)
	/////////////////////////////////////////////////////////////////////////////////////
	dir = directory;
	std::string dir2 = directory;
	dir += "RouteCheckerOptional.csv";
	dir2 += "RouteCheckerMandatory.csv";
	io::CSVReader<5, io::trim_chars<' '>, io::no_quote_escape<','>> in2(dir);
	io::CSVReader<5, io::trim_chars<' '>, io::no_quote_escape<','>> in3(dir2);
	in2.read_header(io::ignore_extra_column, "Dep", "Dest", "Evenodd", "Restriction", "Route");
	std::string Dep, Dest, evenodd, LevelR, Routing;
	while (in2.read_row(Dep, Dest, evenodd, LevelR, Routing))
	{
		auto temp = RouteTo(Dep, Dest, evenodd, LevelR, Routing);
		auto depicao = temp.mDEPICAO;
		RouteData dt;
		std::pair<std::string, RouteData> mypair(depicao, dt);
		routedataoptional.insert(mypair);
		routedataoptional.at(depicao).Routes.push_back(temp);
		routedataoptional.at(depicao).icaos.push_back(Dest);
	}
	LOG_F(INFO, "Route file optional parsed successfully.");
	/////////////////////////////////////////////////////////
	//mandatory
	///////////////////////////////////////////////////////
	in3.read_header(io::ignore_extra_column, "Dep", "Dest", "Evenodd", "Restriction", "MandatoryRoute");
	Dep.erase();
	Dest.erase();
	evenodd.erase();
	LevelR.erase();
	Routing.erase();
	while (in3.read_row(Dep, Dest, evenodd, LevelR, Routing))
	{
		auto temp = RouteTo(Dep, Dest, evenodd, LevelR, Routing);
		auto depicao = temp.mDEPICAO;
		RouteData dt;
		std::pair<std::string, RouteData> mypair(depicao, dt);
		routedatamandatory.insert(mypair);
		routedatamandatory.at(depicao).Routes.push_back(temp);
		routedatamandatory.at(depicao).icaos.push_back(Dest);
	}
	LOG_F(INFO, "Route file mandatory parsed successfully.");
	LOG_F(INFO, "Done file reading.");
}
//Destructor
CUAEController::~CUAEController(void)
{
	standmapping.clear();
}

//------------Functions called by Euroscope in various situations-----------------------------
EuroScopePlugIn::CRadarScreen    *CUAEController::OnRadarScreenCreated(const char * sDisplayName,
	bool NeedRadarContent,
	bool GeoReferenced,
	bool CanBeSaved,
	bool CanBeCreated)
{
	if (strcmp(sDisplayName, "Timer") == 0)
		return NULL;
	TimerRadar * myscreen = new TimerRadar(TimerRadar::to_wstring(directory));
	return myscreen;
}
void CUAEController::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan,
	EuroScopePlugIn::CRadarTarget RadarTarget,
	int ItemCode,
	int TagData,
	char sItemString[16],
	int * pColorCode,
	COLORREF * pRGB,
	double * pFontSize)
{
	if (!FlightPlan.IsValid())
		return;
	auto fpdata = FlightPlan.GetFlightPlanData();
	std::string dep = fpdata.GetOrigin();
	int     idx;
	switch (ItemCode) //when Euroscope wants to know the value of these items we return it. Switch by which one it wants
	{
		
		case TAG_ITEM_RECAT: case TAG_ITEM_RECAT_NOSLASH:
		{
			std::string type = FlightPlan.GetFlightPlanData().GetAircraftInfo();
			if (type.find("/") != std::string::npos)
			{
				std::regex re = std::regex(R"(\/(.{4})\/)");
				std::smatch match;
				if (std::regex_search(type, match, re))
				{
					type = match.str(1);
				}
				else
				{
					std::string firsttype = type.substr(0, 4);
					type = firsttype;
				}
			}
			auto found = recatdict.find(type);
			if (found != recatdict.end())
			{
				std::string out = found->second;
				if (ItemCode == TAG_ITEM_RECAT)
					out = "/" + found->second;
				strcpy(sItemString, out.c_str());
				return;
			}
			else
			{
				char wtc = FlightPlan.GetFlightPlanData().GetAircraftWtc();
				std::string temp;
				switch (wtc)
				{
				case 'J':
				{
					temp = "A";
					break;
				}
				case 'H':
				{
					temp = "B";
					break;
				}
				case 'M':
				{
					temp = "D";
					break;
				}
				case 'L':
				{
					temp = "G";
					break;
				}
				}
				std::string out;
				if (ItemCode == TAG_ITEM_RECAT)
					out = "/" + temp;
				else
					out = temp;
				strcpy(sItemString, out.c_str());
			}
			
			break;
		}
		case TAG_ITEM_STAND:
		{
			auto icao = FlightPlan.GetFlightPlanData().GetDestination();
			if (std::find(activeAirports.begin(), activeAirports.end(), icao) == activeAirports.end())
			{
				std::string logstring = "The airport ";
				logstring += icao;
				logstring += " does not have any data setup. Ignoring it.";

				LOG_F(INFO, logstring.c_str());
				return;
			}

			auto fpdata = FlightPlan.GetFlightPlanData();
			std::string remarks = fpdata.GetRemarks();
			auto dest = fpdata.GetDestination();
			std::regex reOMDB = std::regex(R"(\/STAND([A-Z]\d{1,2}))");
			std::regex reOMSJ = std::regex(R"(\/STAND(\d{1,2}[A-Z]?))");
			std::regex reOMAA1 = std::regex(R"(\/STAND(\d{1,3}))");
			std::regex reOMAA2 = std::regex(R"(\/STAND(GA))");
			std::smatch match;
			if ((std::regex_search(remarks, match, reOMDB) && strcmp(dest, "OMDB") == 0) || (std::regex_search(remarks, match, reOMSJ) && strcmp(dest, "OMSJ") == 0) || ((std::regex_search(remarks, match, reOMAA1) || std::regex_search(remarks, match, reOMAA2)) && strcmp(dest, "OMAA") == 0))
			{
				auto stand = match.str(1);
				auto found2 = data.find(icao);

				if (found2 == data.end()) break;
				auto found = found2->second.find(stand);
				if (found != found2->second.end())
				{
					found->second.isAssigned = true;
					auto temp2 = standmapping.find(icao);
					
					if (temp2 == standmapping.end())
					{
						std::unordered_map<std::string, Stand> tempp;
						standmapping.insert(std::make_pair(icao, tempp));
					}
					auto copy = standmapping.at(icao);
					//LOG_F(INFO, std::to_string(standmapping.size()).c_str());
					std::pair<std::string, Stand> temp(FlightPlan.GetCallsign(), found->second);
					copy.insert(temp);
					standmapping.at(icao) = copy;
				}
				auto aircraftposition = FlightPlan.GetCorrelatedRadarTarget().GetPosition().GetPosition();
				if (CUAEController::determineAircraftCat(FlightPlan) > found->second.mSize)
				{
					*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
					*pRGB = RGB(255, 191, 0);
				}
				if (!FlightPlan.GetCorrelatedRadarTarget().IsValid())
				{
					strcpy(sItemString, found->second.number.c_str());
					return;
				}
				if (!found->second.isEmpty && aircraftposition.DistanceTo(found->second.position) >= TOL)
				{
					auto temp1 = aircraftposition.DistanceTo(found->second.position);
					auto temp2 = aircraftposition.m_Latitude;
					std::string logstring = "Duplicate stand detected. Distance: ";
					logstring += std::to_string(temp1);
					logstring += ". Latitude of aircraft: ";
					logstring += std::to_string(temp2);

					LOG_F(INFO, logstring.c_str());
					*pColorCode = EuroScopePlugIn::TAG_COLOR_EMERGENCY;
				}
				
				strcpy(sItemString, found->second.number.c_str());
				return;
			}
			break;

		}
		case TAG_ITEM_ROUTE_VALID:
		{
			auto fpdata = FlightPlan.GetFlightPlanData();
			if (FlightPlan.GetClearenceFlag())
			{
				std::string logstring = "Aircraft ";
				logstring += FlightPlan.GetCallsign();
				logstring += "is skipped because Clearance flag is set.";
				LOG_F(INFO, logstring.c_str());
				return;
			}
			auto test = fpdata.GetPlanType();
			if (strcmp(test, "V") == 0)
			{
				strcpy(sItemString, "");
				return;
			}
			std::string icaodest = fpdata.GetDestination();
			std::string icaodep = fpdata.GetOrigin();
			std::string icaodesttype = getRouteRegion(routedatamandatory,icaodep, icaodest);
			if (icaodesttype == "?")
			{
				strcpy(sItemString, "?");
				return;
			}
			auto dt = routedatamandatory.at(icaodep).getDatafromICAO(icaodesttype);
			std::string validmandatory = isFlightPlanValid(dt, fpdata.GetRoute(), fpdata.GetFinalAltitude());
			*pColorCode = EuroScopePlugIn::TAG_COLOR_EMERGENCY;
			if (validmandatory != "o")
			{
				strcpy(sItemString, validmandatory.c_str());
				return;
			}
			else 
			{
				if (icaodesttype.length() == 4)return;
				icaodesttype = getRouteRegion(routedataoptional, icaodep, icaodest);
				if (icaodesttype == "?") return;
				auto dtoptional = routedataoptional.at(icaodep).getDatafromICAO(icaodesttype);
				std::string validoptional = isFlightPlanValid(dtoptional, fpdata.GetRoute(), fpdata.GetFinalAltitude());
				*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
				*pRGB = RGB(255, 191, 0);
				if (validoptional == "o") return;
				strcpy(sItemString, validoptional.c_str());
				return;
			}
			


			return;
		}
		case TAG_ITEM_CTOT:
		{
			if ((idx = _SelectAcIndex(FlightPlan)) < 0)
			{
				//now we check if another controller assigned a ctot we need the remark string and other data for that

				auto remarks = fpdata.GetRemarks();
				const char* test = NULL;

				//check if the remarks contains the phrase /CTOT
				test = strstr(remarks, "/CTOT");

				if (test)
				{
					CTime temp;
					temp = CTime::GetCurrentTime();
					tm t, t1;
					temp.GetGmtTm(&t);
					temp.GetLocalTm(&t1);
					std::string callsign = FlightPlan.GetCallsign();
					std::string logstring = "We already found a valid CTOT for " + callsign;
					LOG_F(INFO, logstring.c_str());
					//if yes then a controller already assigned a ctot and the estimated dep time on the flightplan is the ctot
					auto ctot = fpdata.GetEstimatedDepartureTime();
					CTOTData newone;


					//calculating the timezone 
					CTimeSpan diff = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec) - CTime(1900 + t1.tm_year, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);

					//casting our string to int
					int input = atoi(ctot);

					//get the minutes (integer divison for the win)
					int test = input - (input / 100) * 100;

					//safeguard for something silly
					if (input >= 2400 || test >= 60) return;

					//construct the new CTOT object and add it to the sequence. We didnt calculate anything here just read the data from the flightplan
					newone.CTOT = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, input / 100, input - (input / 100) * 100, 0) - diff;
					newone.flightplan = FlightPlan;
					newone.TOBT = newone.CTOT - taxitime;
					if (dep == "OMDB")
					{
						m_sequence_OMDB.push_back(newone);
						std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());
					}
					if (dep == "OMSJ")
					{
						m_sequence_OMSJ.push_back(newone);
						std::sort(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end());
					}
					if (dep == "OMDW")
					{
						m_sequence_OMDW.push_back(newone);
						std::sort(m_sequence_OMDW.begin(), m_sequence_OMDW.end());
					}
					if (dep == "OMAA")
					{
						m_sequence_OMDB.push_back(newone);
						std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());
					}


				}
				return;
			}
			CTime CTOT;
			//just take the stored CTOT and format it to look nice
			if (dep == "OMDB")
				CTOT = m_sequence_OMDB[idx].CTOT;
			if (dep == "OMSJ")
				CTOT = m_sequence_OMSJ[idx].CTOT;
			if (dep == "OMDW")
				CTOT = m_sequence_OMDW[idx].CTOT;
			if (dep == "OMAA")
				CTOT = m_sequence_OMAA[idx].CTOT;
			CString temp1;
			tm t;
			CTOT.GetGmtTm(&t);
			temp1.Format("%.4d", t.tm_hour * 100 + t.tm_min);
			strcpy(sItemString, temp1);
			updateListOMDB();
			updateListOMSJ();
			updateListOMDW();
			updateListOMAA();
			break;
		}
		case TAG_ITEM_TOBT:
		{
			if ((idx = _SelectAcIndex(FlightPlan)) < 0)
			{
				//now we check if another controller assigned a ctot we need the remark string and other data for that

				auto remarks = fpdata.GetRemarks();
				const char* test = NULL;

				//check if the remarks contains the phrase /CTOT
				test = strstr(remarks, "/CTOT");

				if (test)
				{
					CTime temp;
					temp = CTime::GetCurrentTime();
					tm t, t1;
					temp.GetGmtTm(&t);
					temp.GetLocalTm(&t1);
					std::string callsign = FlightPlan.GetCallsign();
					std::string logstring = "We already found a valid CTOT for " + callsign;
					LOG_F(INFO, logstring.c_str());
					//if yes then a controller already assigned a ctot and the estimated dep time on the flightplan is the ctot
					auto ctot = fpdata.GetEstimatedDepartureTime();
					CTOTData newone;


					//calculating the timezone 
					CTimeSpan diff = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec) - CTime(1900 + t1.tm_year, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);

					//casting our string to int
					int input = atoi(ctot);

					//get the minutes (integer divison for the win)
					int test = input - (input / 100) * 100;

					//safeguard for something silly
					if (input >= 2400 || test >= 60) return;

					//construct the new CTOT object and add it to the sequence. We didnt calculate anything here just read the data from the flightplan
					newone.CTOT = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, input / 100, input - (input / 100) * 100, 0) - diff;
					newone.flightplan = FlightPlan;
					newone.TOBT = newone.CTOT - taxitime;
					if (dep == "OMDB")
					{
						m_sequence_OMDB.push_back(newone);
						std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());
					}
					if (dep == "OMSJ")
					{
						m_sequence_OMSJ.push_back(newone);
						std::sort(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end());
					}
					if (dep == "OMDW")
					{
						m_sequence_OMDW.push_back(newone);
						std::sort(m_sequence_OMDW.begin(), m_sequence_OMDW.end());
					}
					if (dep == "OMAA")
					{
						m_sequence_OMDB.push_back(newone);
						std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());
					}


				}
				return;
			}
			CTime TOBT;
			if (dep == "OMDB")
				TOBT = m_sequence_OMDB[idx].TOBT;
			if (dep == "OMSJ")
				TOBT = m_sequence_OMSJ[idx].TOBT;
			if (dep == "OMDW")
				TOBT = m_sequence_OMDW[idx].TOBT;
			if (dep == "OMAA")
				TOBT = m_sequence_OMAA[idx].TOBT;
			CTime curr;
			curr = CTime::GetCurrentTime();
			tm currt, currt1;
			curr.GetGmtTm(&currt);
			CString temp1;
			tm t;
			TOBT.GetGmtTm(&t);
			temp1.Format("%.4d", t.tm_hour * 100 + t.tm_min);
			strcpy(sItemString, temp1);
			std::string status = FlightPlan.GetGroundState();
			if (TOBT - curr < CTimeSpan(0, 0, 5, 0) && status.empty())
			{
				if (TOBT + CTimeSpan(0, 0, 5, 0) > curr)
				{
					*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
					*pRGB = RGB(255, 191, 0);
				}
				else
					*pColorCode = EuroScopePlugIn::TAG_COLOR_EMERGENCY;
			}

			break;
		}
		case TAG_ITEM_Sequence:
		{
			if ((idx = _SelectAcIndex(FlightPlan)) < 0)
			{
				//now we check if another controller assigned a ctot we need the remark string and other data for that

				auto remarks = fpdata.GetRemarks();
				const char* test = NULL;

				//check if the remarks contains the phrase /CTOT
				test = strstr(remarks, "/CTOT");

				if (test)
				{
					CTime temp;
					temp = CTime::GetCurrentTime();
					tm t, t1;
					temp.GetGmtTm(&t);
					temp.GetLocalTm(&t1);
					std::string callsign = FlightPlan.GetCallsign();
					std::string logstring = "We already found a valid CTOT for " + callsign;
					LOG_F(INFO, logstring.c_str());
					//if yes then a controller already assigned a ctot and the estimated dep time on the flightplan is the ctot
					auto ctot = fpdata.GetEstimatedDepartureTime();
					CTOTData newone;


					//calculating the timezone 
					CTimeSpan diff = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec) - CTime(1900 + t1.tm_year, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);

					//casting our string to int
					int input = atoi(ctot);

					//get the minutes (integer divison for the win)
					int test = input - (input / 100) * 100;

					//safeguard for something silly
					if (input >= 2400 || test >= 60) return;

					//construct the new CTOT object and add it to the sequence. We didnt calculate anything here just read the data from the flightplan
					newone.CTOT = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, input / 100, input - (input / 100) * 100, 0) - diff;
					newone.flightplan = FlightPlan;
					newone.TOBT = newone.CTOT - taxitime;
					if (dep == "OMDB")
					{
						m_sequence_OMDB.push_back(newone);
						std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());
					}
					if (dep == "OMSJ")
					{
						m_sequence_OMSJ.push_back(newone);
						std::sort(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end());
					}
					if (dep == "OMDW")
					{
						m_sequence_OMDW.push_back(newone);
						std::sort(m_sequence_OMDW.begin(), m_sequence_OMDW.end());
					}
					if (dep == "OMAA")
					{
						m_sequence_OMDB.push_back(newone);
						std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());
					}


				}
				return;
			}
			int seq = 0;
			if (dep == "OMDB")
				seq = m_sequence_OMDB[idx].sequence;
			if (dep == "OMSJ")
				seq = m_sequence_OMSJ[idx].sequence;
			if (dep == "OMDW")
				seq = m_sequence_OMDW[idx].sequence;
			if (dep == "OMAA")
				seq = m_sequence_OMAA[idx].sequence;
			CString temp3;
			temp3.Format("%d", seq);
			strcpy(sItemString, temp3);
			break;
		}
	}
}
std::string CUAEController::isFlightPlanValid(std::vector<RouteTo> dt, std::string Route, int level)
{
	bool routevalid = false;
	bool cruisevalid = false;
	for (auto d : dt) {
		std::string tmp = Route;
		std::regex rule("\\/(.+?)(\\\s+?)");
		tmp = std::regex_replace(tmp, rule, " ");
		if (!routevalid)
		{
			routevalid = d.isRouteValid(tmp);
		}
		else
		{
			cruisevalid = d.isCruiseValid(level);
			if (cruisevalid && routevalid)
			{
				
				return "o";
			}
			else {
				return "L";
			}

		}
		cruisevalid = d.isCruiseValid(level);
		if (routevalid && cruisevalid)
		{
			return "o";

		}

	}
	if (cruisevalid && !routevalid) return "R";
	else if (routevalid && !cruisevalid) return "L";
	else {
		return "X";
	}
}
void  CUAEController::OnTimer(int Counter)
{
	if (Counter % 5 == 0)
	{
		cleanupStands();
	}
}
inline  bool CUAEController::OnCompileCommand(const char * sCommandLine)
{
	//show the t/o sequence list if we type the command
	if (std::strcmp(sCommandLine, ".showtolist") == 0)
	{
		m_TOSequenceList.ShowFpList(true);
		return true;
	}
	if (std::strcmp(sCommandLine, ".showstands") == 0)
	{

		for (auto airport : activeAirports)
		{
			std::string out;
			auto found = data.find(airport);
			for (auto it : found->second)
			{
				if (!it.second.isEmpty || it.second.isAssigned)
					out += it.first;
			}
			DisplayUserMessage("Occupied Stands", airport.c_str(), out.c_str(), true, true, true, true, true);
		}


		return true;
	}
	return false;
}
void CUAEController::OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget rt)
{
	if (rt.IsValid())
	{
		if (rt.GetPosition().GetPressureAltitude() > 1000) return;
		auto position = rt.GetPosition().GetPosition();
		auto stand = getStandOfAircraft(position);
		auto icao = stand.mICAO;
		auto found = data.find(icao);
		if (found == data.end()) return;
		if (stand.number != "ZZZ") {
			found->second.at(stand.number).isEmpty = false;
			auto mystand = found->second.at(stand.number);
			auto code = mystand.mAirlinecode;
			if (code == "UAE")
			{
				for (auto &temp : standsUAE.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "ETD")
			{
				for (auto &temp : standsETD.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "ABY" || code == "PAX")
			{
				for (auto &temp : standsABY.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "PAX" || code == "ABY")
			{
				for (auto &temp : standsPAX.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "CARGO1")
			{
				for (auto &temp : standsCargoSpecial.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "CARGO" || code == "CLC")
			{
				for (auto &temp : standsCARGO.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}

			if (code == "LWC" || code == "CLC")
			{
				for (auto &temp : standsLOWCOST.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "GA")
			{
				for (auto &temp : standsGA.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "VIP")
			{
				for (auto &temp : standsVIP.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
			if (code == "ALL")
			{
				for (auto &temp : standsOverflow.at(icao))
				{
					if (temp.number == mystand.number)
						temp.isEmpty = false;
				}
			}
		}
	}
}
inline void CUAEController::OnFunctionCall(int FunctionId, const char * sItemString, POINT Pt, RECT Area)
{
	//handle our registered functions
	EuroScopePlugIn::CFlightPlan  fp;
	CString                         str;


	// get the flightplan we are dealing with
	fp = FlightPlanSelectASEL();
	auto fpdata = fp.GetFlightPlanData();
	std::string dep = fpdata.GetOrigin();
	int idx = _SelectAcIndex(fp);
	if (!fp.IsValid())
		return;

	// select it from the sequence

	// switch by the function ID
	switch (FunctionId)
	{
	case TAG_FUNC_EDIT:
		OpenPopupEdit(Area,
			TAG_FUNC_MANUAL_FINISH,
			"");
		LOG_F(INFO, "Opening textbox for stand assignment.");
		break;
	case TAG_FUNC_MANUAL_FINISH:
	{
		LOG_F(INFO, "Processing textinput as stand.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			return;
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		auto dest = fp.GetFlightPlanData().GetDestination();
		std::smatch match;
		if (strcmp(dest, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(dest, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(dest, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		LOG_F(INFO, "Aircraft can be modified. We are processing aircraft: ");
		LOG_F(INFO, fp.GetCallsign());
		std::string input = sItemString;
		
		auto found = data.find(dest);
		if (found == data.end()) return;
		auto found2 = found->second.find(input);
		if (found2 == found->second.end()) return;
		std::string logstring;
		logstring = "Stand ";
		logstring += found2->second.number;
		logstring += " found.";
		LOG_F(INFO, logstring.c_str());
		found2->second.isAssigned = true;
		std::string code = found2->second.mAirlinecode;
		auto it = found2->second;
		if (code == "UAE" && standsUAE.find(dest) != standsUAE.end())
		{
			for (auto &temp : standsUAE.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if (code == "ETD" && standsETD.find(dest) != standsETD.end())
		{
			for (auto &temp : standsETD.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if ((code == "PAX" || code == "ABY") && standsPAX.find(dest) != standsPAX.end())
		{
			for (auto &temp : standsPAX.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}

		if ((code == "CARGO" || code == "CLC") && standsCARGO.find(dest) != standsCARGO.end())
		{
			for (auto &temp : standsCARGO.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}

		if ((code == "LWC" || code == "CLC") && standsLOWCOST.find(dest) != standsLOWCOST.end())

		{
			for (auto &temp : standsLOWCOST.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if (code == "GA" && standsGA.find(dest) != standsGA.end())
		{
			for (auto &temp : standsGA.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if (code == "VIP" && standsVIP.find(dest) != standsVIP.end())
		{
			for (auto &temp : standsVIP.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if ((code == "ABY" || code == "PAX") && standsABY.find(dest) != standsABY.end())
		{
			for (auto &temp : standsABY.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if (code == "CARGO1" && standsCargoSpecial.find(dest) != standsCargoSpecial.end())
		{
			for (auto &temp : standsCargoSpecial.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		if (code == "ALL" && standsOverflow.find(dest) != standsOverflow.end())
		{
			for (auto &temp : standsOverflow.at(dest))
			{
				if (temp.number == it.number)
					temp.isAssigned = true;
			}
		}
		auto found3 = standmapping.find(dest);
		std::unordered_map<std::string, Stand> copy;
		if (found3 != standmapping.end())
			copy = found3->second;
		std::pair<std::string, Stand> temp2(fp.GetCallsign(), found2->second);
		copy.insert(temp2);
		if (found3 == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp3(dest, copy);
			standmapping.insert(temp3);
			LOG_F(INFO, "Standmapping was empty");
		}
		else
		{
			standmapping.at(dest) = copy;
			LOG_F(INFO, "Standmapping was not empty");
		}
			
		
		
		remarks += "/STAND" + input;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if(successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;


	}
	case TAG_FUNC_CLEAR:
	{
		auto dest = fp.GetFlightPlanData().GetDestination();
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "")!=0)
			return;
		LOG_F(INFO, "Clear Stand called on aircraft: ");
		LOG_F(INFO, fp.GetCallsign());
		if (std::find(activeAirports.begin(), activeAirports.end(), dest) == activeAirports.end()) return;
		auto airportstanddata = data.find(dest);
		auto temp1 = standmapping.find(dest);
		if (temp1 == standmapping.end()) return;
		auto copy = temp1->second;
		auto stand = copy.find(fp.GetCallsign());
		if (stand == copy.end()) return;
		auto fpdata = fp.GetFlightPlanData();
		LOG_F(INFO, "Standmapping size before clear: ");
		LOG_F(INFO, std::to_string(standmapping.at(dest).size()).c_str());
		copy.erase(fp.GetCallsign());
		standmapping.at(dest) = copy;
		LOG_F(INFO, "Standmapping size after clear: ");
		LOG_F(INFO, std::to_string(standmapping.at(dest).size()).c_str());
		std::string remarks = fpdata.GetRemarks();
		if (strcmp(dest, "OMDB") == 0)
			remarks = std::regex_replace(remarks, std::regex(R"(\/STAND[A-Z]\d{1,2})"), "");
		else if (strcmp(dest, "OMSJ") == 0)
			remarks = std::regex_replace(remarks, std::regex(R"(\/STAND\d{1,2}[A-Z]?)"), "");
		else if (strcmp(dest, "OMAA") == 0)
		{
			remarks = std::regex_replace(remarks, std::regex(R"(\/STAND\d{1,3})"), "");
			remarks = std::regex_replace(remarks, std::regex(R"(\/STANDGA)"), "");
		}

		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");

		return;
	}
	case TAG_FUNC_ASSIGN_POPUP:
	{
		OpenPopupList(Area, "Assign Stand", 1);
		auto dest = fp.GetFlightPlanData().GetDestination();

		AddPopupListElement("Assign Auto", "", TAG_FUNC_ASSIGN_AUTO);
		if (strcmp(dest, "OMDB") == 0)
			AddPopupListElement("Assign UAE", "", TAG_FUNC_ASSIGN_UAE);
		if (strcmp(dest, "OMSJ") == 0)
		{
			AddPopupListElement("Assign ABY", "", TAG_FUNC_ASSIGN_ABY);
			AddPopupListElement("Assign SQC/GEC/UPS", "", TAG_FUNC_ASSIGN_CARGO2);
		}
		if (strcmp(dest, "OMAA") == 0)
			AddPopupListElement("Assign ETD", "", TAG_FUNC_ASSIGN_ETD);
		AddPopupListElement("Assign CARGO", "", TAG_FUNC_ASSIGN_CARGO);
		AddPopupListElement("Assign PAX", "", TAG_FUNC_ASSIGN_PAX);
		AddPopupListElement("Assign LOWCOST", "", TAG_FUNC_ASSIGN_LOWCOST);
		AddPopupListElement("Assign VIP", "", TAG_FUNC_ASSIGN_VIP);
		AddPopupListElement("Assign GA", "", TAG_FUNC_ASSIGN_GA);
		AddPopupListElement("Clear", "", TAG_FUNC_CLEAR);
		LOG_F(INFO, "Opening stand assignment dialog.");
		break;
	}
	case TAG_FUNC_ASSIGN_AUTO:
	{
		LOG_F(INFO, "Auto assign selected.");
		std::string callsign = fp.GetCallsign();
		std::string remarks = fp.GetFlightPlanData().GetRemarks();
		if (remarks.find("Cargo") != std::string::npos || remarks.find("CARGO") != std::string::npos || remarks.find("cargo") != std::string::npos || remarks.find("freight") != std::string::npos || remarks.find("Freight") != std::string::npos || remarks.find("FREIGHT") != std::string::npos)
		{
			std::string logstring;
			logstring = "Cargo Callsign through remarks for " + callsign + ", because remarks are " + remarks;
			LOG_F(INFO, logstring.c_str());
			goto CARGO;
		}
		if (callsign.length() < 3)
		{
			LOG_F(INFO, "Callsign less then 3 characters. Treathing as PAX.");
			goto PAX;
		}
		std::string op = callsign.substr(0, 3);
		std::regex number = std::regex(R"(.*\d.*)");
		std::smatch match;
		auto test = fp.GetFlightPlanData().GetPlanType();
		if (std::regex_search(op, match, number) || strcmp(test, "V") == 0)
		{
			std::string logstring;
			logstring = "Assigning GA stand for " + callsign + " because we found a number in the first three characters of the callsign or the flightrules are VFR.";
			LOG_F(INFO, logstring.c_str());
			goto GA;
		}
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto found2 = callsignmap.find(icao);
		if (found2 == callsignmap.end()) return;
		auto found = found2->second.find(op);
		if (found != found2->second.end())
		{
			auto assignment = found->second;
			if (assignment == "UAE")
			{
				std::regex uaecargo = std::regex(R"(UAE9\d{3})");
				std::smatch match;
				if (std::regex_search(callsign, match, uaecargo))
				{
					std::string logstring;
					logstring = "Detected SkyCargo for Callsign " + callsign;
					LOG_F(INFO, logstring.c_str());
					goto CARGO;
				}
				std::string logstring;
				logstring = "Detected Emirates Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto UAE;
			}
			if (assignment == "LWC")
			{
				std::string logstring;
				logstring = "Detected lowcost Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto LWC;
			}
			if (assignment == "ETD")
			{
				std::string logstring;
				logstring = "Detected Etihad Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto ETD;
			}
			if (assignment == "ABY")
			{
				std::string logstring;
				logstring = "Detected Air Arabia Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto ABY;
			}
			if (assignment == "VIP")
			{
				std::string logstring;
				logstring = "Detected VIP Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto VIP;
			}
			if (assignment == "CARGO")
			{
				std::string logstring;
				logstring = "Detected Cargo Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto CARGO;
			}
			if (assignment == "CARGO1")
			{
				std::string logstring;
				logstring = "Detected Special Cargo Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto CARGO1;
			}
			if (assignment == "GA")
			{
				std::string logstring;
				logstring = "Detected GA Callsign " + callsign;
				LOG_F(INFO, logstring.c_str());
				goto GA;
			}
		}
		std::string logstring;
		logstring = "Could not find any rule to assign " + callsign + " so we treated it as a normal international carrier.";
		LOG_F(INFO, logstring.c_str());
		goto PAX;
		break;
	}
	case TAG_FUNC_ASSIGN_CARGO:
	{
	CARGO:
		LOG_F(INFO, "Cargo assignment in progress.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(icao, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		std::string logstring = "Processing aircraft: ";
		logstring += fp.GetCallsign();
		LOG_F(INFO, logstring.c_str());
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			LOG_F(INFO, "Standmapping for destination is not empty.");
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				return;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		logstring.clear();
		logstring = "Aircraft parking code is ";
		logstring += size;
		LOG_F(INFO, logstring.c_str());
		auto standshere = standsCARGO.find(icao);
		if (standshere == standsCARGO.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		logstring.clear();
		logstring = "Valid stand returned. It is ";
		logstring += stand.number;
		LOG_F(INFO, logstring.c_str());
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			LOG_F(INFO, "Standmapping for destination is empty.");
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsCARGO.at(icao))
		{
			if (temp.number == stand.number)
			{
				LOG_F(INFO, "Stand found in CARGO list.");
				temp.isAssigned = true;
				break;
			}
		}
		for (auto &temp : standsLOWCOST.at(icao))
		{
			if (temp.number == stand.number)
			{
				LOG_F(INFO, "Stand found in LOWCOST list.");
				temp.isAssigned = true;
				break;
			}
		}
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;
		
	}
	case TAG_FUNC_ASSIGN_PAX:
	{
	PAX:
		LOG_F(INFO, "PAX assignment in progress.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(icao, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		std::string logstring = "Processing aircraft: ";
		logstring += fp.GetCallsign();
		LOG_F(INFO, logstring.c_str());
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			LOG_F(INFO, "Standmapping for destination is not empty.");
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		logstring.clear();
		logstring = "Aircraft parking code is ";
		logstring += size;
		LOG_F(INFO, logstring.c_str());
		auto standshere = standsPAX.find(icao);
		if (standshere == standsPAX.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			return;
		logstring.clear();
		logstring = "Valid stand returned. It is ";
		logstring += stand.number;
		LOG_F(INFO, logstring.c_str());
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsPAX.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		for (auto &temp : standsABY.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;
	}
	case TAG_FUNC_ASSIGN_UAE:
	{
	UAE:
		LOG_F(INFO, "UAE assignment in progress.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(icao, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		std::string logstring = "Processing aircraft: ";
		logstring += fp.GetCallsign();
		LOG_F(INFO, logstring.c_str());
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		logstring.clear();
		logstring = "Aircraft parking code is ";
		logstring += size;
		LOG_F(INFO, logstring.c_str());
		auto standshere = standsUAE.find(icao);
		if (standshere == standsUAE.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		logstring.clear();
		logstring = "Valid stand returned. It is ";
		logstring += stand.number;
		LOG_F(INFO, logstring.c_str());
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsUAE.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		
		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;
	}
	case TAG_FUNC_ASSIGN_ABY:
	{
	ABY:
		LOG_F(INFO, "ABY assignment in progress.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(icao, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		std::string logstring = "Processing aircraft: ";
		logstring += fp.GetCallsign();
		LOG_F(INFO, logstring.c_str());
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		logstring.clear();
		logstring = "Aircraft parking code is ";
		logstring += size;
		LOG_F(INFO, logstring.c_str());
		auto standshere = standsABY.find(icao);
		auto standsherePAX = standsPAX.find(icao);
		std::vector<Stand> joined;
		joined.reserve(standshere->second.size() + standsherePAX->second.size());
		joined.insert(joined.end(), standshere->second.begin(), standshere->second.end());
		joined.insert(joined.end(), standsherePAX->second.begin(), standsherePAX->second.end());
		if (standshere == standsABY.end()) break;
		//ABY parks everywhere in sharjah
		auto stand = extractRandomStand(joined, size, icao);
		if (stand.number == "Z00")
			break;
		logstring.clear();
		logstring = "Valid stand returned. It is ";
		logstring += stand.number;
		LOG_F(INFO, logstring.c_str());
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsABY.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		for (auto &temp : standsPAX.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}

		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;
	}
	case TAG_FUNC_ASSIGN_VIP:
	{
	VIP:
		LOG_F(INFO, "VIP assignment in progress.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(icao, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		std::string logstring = "Processing aircraft: ";
		logstring += fp.GetCallsign();
		LOG_F(INFO, logstring.c_str());
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		logstring.clear();
		logstring = "Aircraft parking code is ";
		logstring += size;
		LOG_F(INFO, logstring.c_str());
		auto standshere = standsVIP.find(icao);
		if (standshere == standsVIP.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		logstring.clear();
		logstring = "Valid stand returned. It is ";
		logstring += stand.number;
		LOG_F(INFO, logstring.c_str());
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsVIP.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}

		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;
	}
	case TAG_FUNC_ASSIGN_LOWCOST:
	{
	LWC:
		LOG_F(INFO, "LOWCOST assignment in progress.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(icao, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		std::string logstring = "Processing aircraft: ";
		logstring += fp.GetCallsign();
		LOG_F(INFO, logstring.c_str());
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		logstring.clear();
		logstring = "Aircraft parking code is ";
		logstring += size;
		LOG_F(INFO, logstring.c_str());
		auto standshere = standsLOWCOST.find(icao);
		if (standshere == standsLOWCOST.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		logstring.clear();
		logstring = "Valid stand returned. It is ";
		logstring += stand.number;
		LOG_F(INFO, logstring.c_str());
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsCARGO.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}
		for (auto &temp : standsLOWCOST.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}

		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;
	}
	case TAG_FUNC_ASSIGN_GA:
	{
	GA:
		LOG_F(INFO, "GA assignment in progress.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(icao, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		std::string logstring = "Processing aircraft: ";
		logstring += fp.GetCallsign();
		LOG_F(INFO, logstring.c_str());
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		logstring.clear();
		logstring = "Aircraft parking code is ";
		logstring += size;
		LOG_F(INFO, logstring.c_str());
		auto standshere = standsGA.find(icao);
		if (standshere == standsGA.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		logstring.clear();
		logstring = "Valid stand returned. It is ";
		logstring += stand.number;
		LOG_F(INFO, logstring.c_str());
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsGA.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}

		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;
	}
	case TAG_FUNC_ASSIGN_ETD:
	{
	ETD:
		LOG_F(INFO, "ETD assignment in progress.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(icao, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		std::string logstring = "Processing aircraft: ";
		logstring += fp.GetCallsign();
		LOG_F(INFO, logstring.c_str());
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		logstring.clear();
		logstring = "Aircraft parking code is ";
		logstring += size;
		LOG_F(INFO, logstring.c_str());
		auto standshere = standsETD.find(icao);
		if (standshere == standsETD.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		logstring.clear();
		logstring = "Valid stand returned. It is ";
		logstring += stand.number;
		LOG_F(INFO, logstring.c_str());
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsETD.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}

		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;
	}
	case TAG_FUNC_ASSIGN_CARGO2:
	{
	CARGO1:
		LOG_F(INFO, "Special Cargo assignment in progress.");
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		auto icao = fp.GetFlightPlanData().GetDestination();
		auto fpdata = fp.GetFlightPlanData();
		std::string remarks = fpdata.GetRemarks();
		std::smatch match;
		if (strcmp(icao, "OMDB") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND[A-Z]\d{1,2})")))
			return;
		if (strcmp(icao, "OMSJ") == 0 && std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,2}[A-Z]?)")))
			return;
		if (strcmp(icao, "OMAA") == 0 && (std::regex_search(remarks, match, std::regex(R"(\/STAND\d{1,3})")) || std::regex_search(remarks, match, std::regex(R"(\/STANDGA)"))))
			return;
		std::string logstring = "Processing aircraft: ";
		logstring += fp.GetCallsign();
		LOG_F(INFO, logstring.c_str());
		auto found = standmapping.find(icao);
		std::unordered_map<std::string, Stand> copy;
		if (found != standmapping.end())
		{
			auto found2 = found->second.find(fp.GetCallsign());
			if (found2 != found->second.end())
				break;
			copy = standmapping.at(icao);
		}
		auto size = determineAircraftCat(fp);
		logstring.clear();
		logstring = "Aircraft parking code is ";
		logstring += size;
		LOG_F(INFO, logstring.c_str());
		auto standshere = standsCargoSpecial.find(icao);
		if (standshere == standsCargoSpecial.end()) break;
		auto stand = extractRandomStand(standshere->second, size, icao);
		if (stand.number == "Z00")
			break;
		logstring.clear();
		logstring = "Valid stand returned. It is ";
		logstring += stand.number;
		LOG_F(INFO, logstring.c_str());
		data.at(icao).at(stand.number).isAssigned = true;
		std::pair<std::string, Stand> temp(fp.GetCallsign(), stand);
		copy.insert(temp);
		if (found == standmapping.end())
		{
			std::pair<std::string, std::unordered_map<std::string, Stand>> temp2(icao, copy);
			standmapping.insert(temp2);
		}
		else
			standmapping.at(icao) = copy;
		for (auto &temp : standsVIP.at(icao))
		{
			if (temp.number == stand.number)
				temp.isAssigned = true;
		}

		remarks += "/STAND" + stand.number;
		fpdata.SetRemarks(remarks.c_str());
		bool successful = fpdata.AmendFlightPlan();
		if (successful)
			LOG_F(INFO, "FP amend successful");
		else
			LOG_F(INFO, "FP amend NOT successful");
		break;
	}
	case TAG_FUNC_ROUTING:
	{
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		std::string handlername = "Mandatory route for ";
		handlername += fp.GetCallsign();
		std::string dest = fpdata.GetDestination();
		auto routes = routedatamandatory[fpdata.GetOrigin()].getDatafromICAO(dest);
		if (routes.empty())
		{
			routes = routedatamandatory[fpdata.GetOrigin()].getDatafromICAO(dest.substr(0, 2));
			if (routes.empty())
			{
				routes = routedatamandatory[fpdata.GetOrigin()].getDatafromICAO(dest.substr(0, 1));
				if (routes.empty())
					DisplayUserMessage(handlername.c_str(), "", "No route found", true, true, true, true, true);
			}

		}
		for (auto temp : routes)
		{
			std::string routing = "Valid routes to ";
			routing += dest;
			routing += ": ";
			routing += temp.mRoute;
			routing += ". Flightlevel is ";
			if (temp.mLevelR.empty())
				routing += "not restricted";
			else
			{
				routing += "restricted to ";
				routing += temp.mLevelR;
			}
			routing += ". The direction of flight dictates an ";
			routing += temp.mEvenOdd;
			routing += " cruise level.";
			DisplayUserMessage(handlername.c_str(), "", routing.c_str(), true, true, true, true, true);
		}

		break;
	}
	case TAG_FUNC_ROUTING_OPT:
	{
		if (!fp.GetTrackingControllerIsMe() && strcmp(fp.GetTrackingControllerCallsign(), "") != 0)
			break;
		std::string handlername = "Optional route for ";
		handlername += fp.GetCallsign();
		std::string dest = fpdata.GetDestination();
		auto routes = routedataoptional[fpdata.GetOrigin()].getDatafromICAO(dest);
		if (routes.empty())
		{
			routes = routedataoptional[fpdata.GetOrigin()].getDatafromICAO(dest.substr(0, 2));
			if (routes.empty())
			{
				routes = routedataoptional[fpdata.GetOrigin()].getDatafromICAO(dest.substr(0, 1));
				if (routes.empty())
					DisplayUserMessage(handlername.c_str(), "", "No route found", true, true, true, true, true);
			}

		}
		for (auto temp : routes)
		{
			std::string routing = "Valid routes to ";
			routing += dest;
			routing += ": ";
			routing += temp.mRoute;
			routing += ". Flightlevel is ";
			if (temp.mLevelR.empty())
				routing += "not restricted";
			else
			{
				routing += "restricted to ";
				routing += temp.mLevelR;
			}
			routing += ". The direction of flight dictates an ";
			routing += temp.mEvenOdd;
			routing += " cruise level.";
			DisplayUserMessage(handlername.c_str(), "", routing.c_str(), true, true, true, true, true);
		}

		break;
	}
	case TAG_FUNC_CTOT_MANUAL: // TAG function
	{

		// open a popup editor there
		OpenPopupEdit(Area,
			TAG_FUNC_CTOT_MANUAL_FINISH,
			"");
		break;
	}
	case TAG_FUNC_CTOT_MANUAL_FINISH: // when finished editing
	{
		//same thing as in getTagItem
		CTime temp;
		temp = CTime::GetCurrentTime();
		tm t, t1;
		temp.GetGmtTm(&t);
		temp.GetLocalTm(&t1);
		CTimeSpan diff = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec) - CTime(1900 + t1.tm_year, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);
		//as our ctot we use the manually entered 4 digits
		int input = atoi(sItemString);
		int test = input - (input / 100) * 100;
		if (input >= 2400 || test >= 60) return;
		auto data = fp.GetFlightPlanData();
		auto remarks = data.GetRemarks();
		const char* test1 = NULL;
		std::string temp2 = remarks;
		test1 = strstr(remarks, "/CTOT");
		//if /CTOT is not present in the remarks we append it to it
		if (!test1)
		{
			std::string temp2;
			temp2 = remarks;
			temp2 += "/CTOT";
			data.SetRemarks(temp2.c_str());
			data.AmendFlightPlan();
		}
		// if the aircraft was in the sequence already we modify its CTOTData otherwise we make a new one
		if (idx >= 0)
		{
			if (dep == "OMDB")
			{
				try {
					m_sequence_OMDB[idx].CTOT = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, input / 100, input - (input / 100) * 100, 0) - diff;
					m_sequence_OMDB[idx].TOBT = m_sequence_OMDB[idx].CTOT - taxitime;
					m_sequence_OMDB[idx].manual = true;
					std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());

				}
				catch (...)
				{
				}
			}
			if (dep == "OMSJ")
			{
				try {
					m_sequence_OMSJ[idx].CTOT = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, input / 100, input - (input / 100) * 100, 0) - diff;
					m_sequence_OMSJ[idx].TOBT = m_sequence_OMSJ[idx].CTOT - taxitime;
					m_sequence_OMSJ[idx].manual = true;
					std::sort(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end());

				}
				catch (...)
				{
				}
			}
			if (dep == "OMDW")
			{
				try {
					m_sequence_OMDW[idx].CTOT = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, input / 100, input - (input / 100) * 100, 0) - diff;
					m_sequence_OMDW[idx].TOBT = m_sequence_OMDW[idx].CTOT - taxitime;
					m_sequence_OMDW[idx].manual = true;
					std::sort(m_sequence_OMDW.begin(), m_sequence_OMDW.end());

				}
				catch (...)
				{
				}
			}
			if (dep == "OMAA")
			{
				try {
					m_sequence_OMAA[idx].CTOT = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, input / 100, input - (input / 100) * 100, 0) - diff;
					m_sequence_OMAA[idx].TOBT = m_sequence_OMAA[idx].CTOT - taxitime;
					m_sequence_OMAA[idx].manual = true;
					std::sort(m_sequence_OMAA.begin(), m_sequence_OMAA.end());

				}
				catch (...)
				{
				}
			}
		}
		else {
			CTOTData temp1;
			temp1.flightplan = fp;
			try {
				temp1.CTOT = CTime(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, input / 100, input - (input / 100) * 100, 0) - diff;
				temp1.TOBT = temp1.CTOT - taxitime;
				temp1.manual = true;
				if (dep == "OMDB")
				{
					m_sequence_OMDB.push_back(temp1);
					std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());
				}
				if (dep == "OMSJ")
				{
					m_sequence_OMSJ.push_back(temp1);
					std::sort(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end());
				}
				if (dep == "OMDW")
				{
					m_sequence_OMDW.push_back(temp1);
					std::sort(m_sequence_OMDW.begin(), m_sequence_OMDW.end());
				}
				if (dep == "OMAA")
				{
					m_sequence_OMAA.push_back(temp1);
					std::sort(m_sequence_OMAA.begin(), m_sequence_OMAA.end());
				}
			}
			catch (...)
			{
			}
		}
		// at last we modify the estimated departure time in the flightplan so other controller's plugin can parse the value themselves
		idx = _SelectAcIndex(fp);
		auto fpdata = fp.GetFlightPlanData();
		tm t2 = { 0 };
		if (dep == "OMDB")
			m_sequence_OMDB[idx].CTOT.GetGmtTm(&t2);
		if (dep == "OMSJ")
			m_sequence_OMSJ[idx].CTOT.GetGmtTm(&t2);
		if (dep == "OMDW")
			m_sequence_OMDW[idx].CTOT.GetGmtTm(&t2);
		if (dep == "OMAA")
			m_sequence_OMAA[idx].CTOT.GetGmtTm(&t2);
		CString cstring;
		cstring.Format("%.4d", t2.tm_hour * 100 + t2.tm_min);
		fpdata.SetEstimatedDepartureTime(cstring);
		fpdata.AmendFlightPlan();
		//we recalculate the CTOT for all aircraft in sequence after the manually assigned one
		break;
	}
	case TAG_FUNC_CTOT_ASSIGN: // TAG function
	{
		// start a popup list
		OpenPopupList(Area, "Assign CTOT", 1);


		AddPopupListElement("Assign in sequence", "", TAG_FUNC_CTOT_ASSIGN_SEQ);
		AddPopupListElement("Assign ASAP", "", TAG_FUNC_CTOT_ASSIGN_ASAP);


		// finally add a fixed element to clear the ctot

		AddPopupListElement("Clear", "",
			TAG_FUNC_CTOT_CLEAR);
		break;
	}
	case TAG_FUNC_CTOT_ASSIGN_SEQ: // user selected assign in sequence
	{
		//dont assign if we already have assigned one. Safeguards against missclicks
		if (idx >= 0) return;
		// get the remarks and modify it
		auto data = fp.GetFlightPlanData();
		auto remarks = data.GetRemarks();
		const char* test = NULL;
		std::string temp = remarks;
		test = strstr(remarks, "/CTOT");
		if (!test)
		{
			std::string temp;
			temp = remarks;
			temp += "/CTOT";
			data.SetRemarks(temp.c_str());
			data.AmendFlightPlan();
		}
		CUAEController::assignCTOT(false, fp);

		//again modify the estimated departure time in the flightplan
		idx = _SelectAcIndex(fp);
		auto fpdata = fp.GetFlightPlanData();
		std::string dep = fpdata.GetOrigin();
		tm t;
		if (dep == "OMDB")
			m_sequence_OMDB[idx].CTOT.GetGmtTm(&t);
		if (dep == "OMSJ")
			m_sequence_OMSJ[idx].CTOT.GetGmtTm(&t);
		if (dep == "OMDW")
			m_sequence_OMDW[idx].CTOT.GetGmtTm(&t);
		if (dep == "OMAA")
			m_sequence_OMAA[idx].CTOT.GetGmtTm(&t);
		CString cstring;
		cstring.Format("%.4d", t.tm_hour * 100 + t.tm_min);
		fpdata.SetEstimatedDepartureTime(cstring);
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_CTOT_ASSIGN_ASAP: // user selected asap
	{
		//all is the same but we call assignCTOT with the asap flag
		if (idx >= 0) return;
		auto data = fp.GetFlightPlanData();
		auto remarks = data.GetRemarks();
		const char* test = NULL;
		std::string temp = remarks;
		test = strstr(remarks, "/CTOT");
		if (!test)
		{
			std::string temp;
			temp = remarks;
			temp += "/CTOT";
			data.SetRemarks(temp.c_str());
			data.AmendFlightPlan();
		}
		CUAEController::assignCTOT(true, fp);
		idx = _SelectAcIndex(fp);
		auto fpdata = fp.GetFlightPlanData();
		std::string dep = fpdata.GetOrigin();
		tm t;
		if (dep == "OMDB")
			m_sequence_OMDB[idx].CTOT.GetGmtTm(&t);
		if (dep == "OMSJ")
			m_sequence_OMSJ[idx].CTOT.GetGmtTm(&t);
		if (dep == "OMDW")
			m_sequence_OMDW[idx].CTOT.GetGmtTm(&t);
		if (dep == "OMAA")
			m_sequence_OMAA[idx].CTOT.GetGmtTm(&t);
		CString cstring;
		cstring.Format("%.4d", t.tm_hour * 100 + t.tm_min);
		fpdata.SetEstimatedDepartureTime(cstring);
		fpdata.AmendFlightPlan();
		break;
	}
	case TAG_FUNC_CTOT_CLEAR: // clear the ctot
	{
		// simply clear
		if (idx < 0) break;
		auto cadata = fp.GetFlightPlanData();
		std::string dep = cadata.GetOrigin();
		auto remarks = cadata.GetRemarks();
		const char* test = NULL;
		std::string temp = remarks;
		test = strstr(remarks, "/CTOT");
		if (test)
		{
			//find and replace /CTOT from remarks
			temp = std::regex_replace(temp, std::regex("\\/CTOT"), "");
			cadata.SetRemarks(temp.c_str());
			cadata.AmendFlightPlan();
		}
		if (dep == "OMDB")
			m_sequence_OMDB.erase(std::remove(m_sequence_OMDB.begin(), m_sequence_OMDB.end(), m_sequence_OMDB.at(_SelectAcIndex(fp))), m_sequence_OMDB.end());
		if (dep == "OMSJ")
			m_sequence_OMSJ.erase(std::remove(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end(), m_sequence_OMSJ.at(_SelectAcIndex(fp))), m_sequence_OMSJ.end());
		if (dep == "OMDW")
			m_sequence_OMDW.erase(std::remove(m_sequence_OMDW.begin(), m_sequence_OMDW.end(), m_sequence_OMDW.at(_SelectAcIndex(fp))), m_sequence_OMDW.end());
		if (dep == "OMAA")
			m_sequence_OMAA.erase(std::remove(m_sequence_OMAA.begin(), m_sequence_OMAA.end(), m_sequence_OMAA.at(_SelectAcIndex(fp))), m_sequence_OMAA.end());
		m_TOSequenceList.RemoveFpFromTheList(fp);
		updateListOMDB();
		updateListOMSJ();
		updateListOMDW();
		updateListOMAA();
		if (dep == "OMDB")
		{
			if (m_sequence_OMDB.size() == idx) break;
			if (idx == 0) recalculateCTOT(*m_sequence_OMDB.begin());
			else recalculateCTOT(m_sequence_OMDB[idx - 1]);
			break;
		}
		if (dep == "OMSJ")
		{
			if (m_sequence_OMSJ.size() == idx) break;
			if (idx == 0) recalculateCTOT(*m_sequence_OMSJ.begin());
			else recalculateCTOT(m_sequence_OMSJ[idx - 1]);
			break;
		}
		if (dep == "OMDW")
		{
			if (m_sequence_OMDW.size() == idx) break;
			if (idx == 0) recalculateCTOT(*m_sequence_OMDW.begin());
			else recalculateCTOT(m_sequence_OMDW[idx - 1]);
			break;
		}
		if (dep == "OMAA")
		{
			if (m_sequence_OMAA.size() == idx) break;
			if (idx == 0) recalculateCTOT(*m_sequence_OMAA.begin());
			else recalculateCTOT(m_sequence_OMAA[idx - 1]);
			break;
		}
		break;
	}

	}// switch by the function ID
}
//--------------------------Helper functions-----------------------------------------
Stand CUAEController::getStandOfAircraft(EuroScopePlugIn::CPosition position)
{
	for (auto airport : activeAirports)
	{
		auto found = data.find(airport);
		if (found == data.end()) continue;
		for (auto stand : found->second)
		{
			auto distance = position.DistanceTo(stand.second.position);
			if (distance < TOL)
				return stand.second;
			if (distance > 5)
				break;
		}
	}
	Stand empty("ZZZ", "N000-00-00.0", "E000-00-00.0", "PAX", "", "", "F", "yes", "OMDB");
	return empty;
}
void CUAEController::assignCTOT(bool asap, EuroScopePlugIn::CFlightPlan flightplan)
{
	CTime time = CTime::GetCurrentTime();
	CTime ctot;
	CTime tobt;
	CTOTData temp;
	auto cadata = flightplan.GetFlightPlanData();
	std::string dep = cadata.GetOrigin();
	//if there is none in sequence or we want asap we dont care about separation
	if (dep == "OMDB")
	{
		if (m_sequence_OMDB.empty() || asap)
		{

			temp.flightplan = flightplan;
			temp.sequence = -1;
			ctot = time + CTimeSpan(0, 0, 30, 0);
			tobt = ctot - taxitime;
			temp.CTOT = ctot;
			temp.TOBT = tobt;

		}
		else
		{
			//get the last aircraft in sequence
			CTOTData &end = *(m_sequence_OMDB.end() - 1);
			CTimeSpan increment = getIncrement(end.flightplan, flightplan);
			temp.flightplan = flightplan;
			temp.sequence = -1;
			//assign at earliest 30 minutes after now 
			ctot = std::max(time + CTimeSpan(0, 0, 30, 0), end.CTOT + increment);
			tobt = ctot - taxitime;
			temp.CTOT = ctot;
			temp.TOBT = tobt;


		}

		m_sequence_OMDB.push_back(temp);
		std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());
		if (asap)
		{
			if (int ind = _SelectAcIndex(temp.flightplan) > 0) recalculateCTOT(m_sequence_OMDB.at(ind - 1));
			else recalculateCTOT(*(m_sequence_OMDB.begin()));

		}
	}
	if (dep == "OMSJ")
	{
		if (m_sequence_OMSJ.empty() || asap)
		{

			temp.flightplan = flightplan;
			temp.sequence = -1;
			ctot = time + CTimeSpan(0, 0, 30, 0);
			tobt = ctot - taxitime;
			temp.CTOT = ctot;
			temp.TOBT = tobt;

		}
		else
		{
			//get the last aircraft in sequence
			CTOTData &end = *(m_sequence_OMSJ.end() - 1);
			CTimeSpan increment = getIncrement(end.flightplan, flightplan);
			temp.flightplan = flightplan;
			temp.sequence = -1;
			//assign at earliest 30 minutes after now 
			ctot = std::max(time + CTimeSpan(0, 0, 30, 0), end.CTOT + increment);
			tobt = ctot - taxitime;
			temp.CTOT = ctot;
			temp.TOBT = tobt;


		}

		m_sequence_OMSJ.push_back(temp);
		std::sort(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end());
		if (asap)
		{
			if (int ind = _SelectAcIndex(temp.flightplan) > 0) recalculateCTOT(m_sequence_OMSJ.at(ind - 1));
			else recalculateCTOT(*(m_sequence_OMSJ.begin()));

		}
	}
	if (dep == "OMDW")
	{
		if (m_sequence_OMDW.empty() || asap)
		{

			temp.flightplan = flightplan;
			temp.sequence = -1;
			ctot = time + CTimeSpan(0, 0, 30, 0);
			tobt = ctot - taxitime;
			temp.CTOT = ctot;
			temp.TOBT = tobt;

		}
		else
		{
			//get the last aircraft in sequence
			CTOTData &end = *(m_sequence_OMDW.end() - 1);
			CTimeSpan increment = getIncrement(end.flightplan, flightplan);
			temp.flightplan = flightplan;
			temp.sequence = -1;
			//assign at earliest 30 minutes after now 
			ctot = std::max(time + CTimeSpan(0, 0, 30, 0), end.CTOT + increment);
			tobt = ctot - taxitime;
			temp.CTOT = ctot;
			temp.TOBT = tobt;


		}

		m_sequence_OMDW.push_back(temp);
		std::sort(m_sequence_OMDW.begin(), m_sequence_OMDW.end());
		if (asap)
		{
			if (int ind = _SelectAcIndex(temp.flightplan) > 0) recalculateCTOT(m_sequence_OMDW.at(ind - 1));
			else recalculateCTOT(*(m_sequence_OMDW.begin()));

		}
	}
	if (dep == "OMAA")
	{
		if (m_sequence_OMAA.empty() || asap)
		{

			temp.flightplan = flightplan;
			temp.sequence = -1;
			ctot = time + CTimeSpan(0, 0, 30, 0);
			tobt = ctot - taxitime;
			temp.CTOT = ctot;
			temp.TOBT = tobt;

		}
		else
		{
			//get the last aircraft in sequence
			CTOTData &end = *(m_sequence_OMAA.end() - 1);
			CTimeSpan increment = getIncrement(end.flightplan, flightplan);
			temp.flightplan = flightplan;
			temp.sequence = -1;
			//assign at earliest 30 minutes after now 
			ctot = std::max(time + CTimeSpan(0, 0, 30, 0), end.CTOT + increment);
			tobt = ctot - taxitime;
			temp.CTOT = ctot;
			temp.TOBT = tobt;


		}

		m_sequence_OMAA.push_back(temp);
		std::sort(m_sequence_OMAA.begin(), m_sequence_OMAA.end());
		if (asap)
		{
			if (int ind = _SelectAcIndex(temp.flightplan) > 0) recalculateCTOT(m_sequence_OMAA.at(ind - 1));
			else recalculateCTOT(*(m_sequence_OMAA.begin()));

		}
	}
}
void CUAEController::updateListOMDB()
{
	//some housekeeping 
	for (CTOTData i : m_sequence_OMDB)
	{
		//add all aircraft in sequence to the euroscope list
		m_TOSequenceList.AddFpToTheList(i.flightplan);
	}
	for (CTOTData i : m_sequence_OMDB)
	{
		EuroScopePlugIn::CFlightPlan first = FlightPlanSelectFirst();
		EuroScopePlugIn::CFlightPlan temp = FlightPlanSelectNext(first);
		bool found = false;
		if (first.GetCallsign() == i.flightplan.GetCallsign()) found = true;
		while (temp.IsValid() && !found)
		{
			if (temp.GetCallsign() == i.flightplan.GetCallsign())
			{
				found = true;
				break;
			}
			temp = FlightPlanSelectNext(temp);
		}
		if (!found)
		{
			m_TOSequenceList.RemoveFpFromTheList(i.flightplan);
			m_sequence_OMDB.erase(std::remove(m_sequence_OMDB.begin(), m_sequence_OMDB.end(), m_sequence_OMDB.at(_SelectAcIndex(i.flightplan))), m_sequence_OMDB.end());
			continue;
		}


		//if any aircraft takes off remove it automatically
		auto fp = i.flightplan;
		EuroScopePlugIn::CRadarTarget rt = fp.GetCorrelatedRadarTarget();
		auto cad = fp.GetFlightPlanData();
		std::string remarks = cad.GetRemarks();
		const char* test = NULL;
		test = strstr(remarks.c_str(), "/CTOT");
		if (rt.GetGS() > 80 || !test)
		{
			remarks = std::regex_replace(remarks, std::regex("\\/CTOT"), "");
			cad.SetRemarks(remarks.c_str());
			cad.AmendFlightPlan();
			m_TOSequenceList.RemoveFpFromTheList(fp);
			m_sequence_OMDB.erase(std::remove(m_sequence_OMDB.begin(), m_sequence_OMDB.end(), m_sequence_OMDB.at(_SelectAcIndex(fp))), m_sequence_OMDB.end());
		}
	}
	std::sort(m_sequence_OMDB.begin(), m_sequence_OMDB.end());
	for (CTOTData &i : m_sequence_OMDB)
	{
		//get the number in sequence for everyone
		i.sequence = _SelectAcIndex(i.flightplan) + 1;
	}
	if (m_sequence_OMDB.empty())
	{
		return;
	}
	recalculateCTOT(*m_sequence_OMDB.begin());
}
void CUAEController::updateListOMSJ()
{
	//some housekeeping 
	for (CTOTData i : m_sequence_OMSJ)
	{
		//add all aircraft in sequence to the euroscope list
		m_TOSequenceList.AddFpToTheList(i.flightplan);
	}
	for (CTOTData i : m_sequence_OMSJ)
	{
		EuroScopePlugIn::CFlightPlan first = FlightPlanSelectFirst();
		EuroScopePlugIn::CFlightPlan temp = FlightPlanSelectNext(first);
		bool found = false;
		if (first.GetCallsign() == i.flightplan.GetCallsign()) found = true;
		while (temp.IsValid() && !found)
		{
			if (temp.GetCallsign() == i.flightplan.GetCallsign())
			{
				found = true;
				break;
			}
			temp = FlightPlanSelectNext(temp);
		}
		if (!found)
		{
			m_TOSequenceList.RemoveFpFromTheList(i.flightplan);
			m_sequence_OMSJ.erase(std::remove(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end(), m_sequence_OMSJ.at(_SelectAcIndex(i.flightplan))), m_sequence_OMSJ.end());
			continue;
		}


		//if any aircraft takes off remove it automatically
		auto fp = i.flightplan;
		EuroScopePlugIn::CRadarTarget rt = fp.GetCorrelatedRadarTarget();
		auto cad = fp.GetFlightPlanData();
		std::string remarks = cad.GetRemarks();
		const char* test = NULL;
		test = strstr(remarks.c_str(), "/CTOT");
		if (rt.GetGS() > 80 || !test)
		{
			remarks = std::regex_replace(remarks, std::regex("\\/CTOT"), "");
			cad.SetRemarks(remarks.c_str());
			cad.AmendFlightPlan();
			m_TOSequenceList.RemoveFpFromTheList(fp);
			m_sequence_OMSJ.erase(std::remove(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end(), m_sequence_OMSJ.at(_SelectAcIndex(fp))), m_sequence_OMSJ.end());
		}
	}
	std::sort(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end());
	for (CTOTData &i : m_sequence_OMSJ)
	{
		//get the number in sequence for everyone
		i.sequence = _SelectAcIndex(i.flightplan) + 1;
	}
	if (m_sequence_OMSJ.empty())
	{
		return;
	}
	recalculateCTOT(*m_sequence_OMSJ.begin());
}
void CUAEController::updateListOMDW()
{
	//some housekeeping 
	for (CTOTData i : m_sequence_OMDW)
	{
		//add all aircraft in sequence to the euroscope list
		m_TOSequenceList.AddFpToTheList(i.flightplan);
	}
	for (CTOTData i : m_sequence_OMDW)
	{
		EuroScopePlugIn::CFlightPlan first = FlightPlanSelectFirst();
		EuroScopePlugIn::CFlightPlan temp = FlightPlanSelectNext(first);
		bool found = false;
		if (first.GetCallsign() == i.flightplan.GetCallsign()) found = true;
		while (temp.IsValid() && !found)
		{
			if (temp.GetCallsign() == i.flightplan.GetCallsign())
			{
				found = true;
				break;
			}
			temp = FlightPlanSelectNext(temp);
		}
		if (!found)
		{
			m_TOSequenceList.RemoveFpFromTheList(i.flightplan);
			m_sequence_OMDW.erase(std::remove(m_sequence_OMDW.begin(), m_sequence_OMDW.end(), m_sequence_OMDW.at(_SelectAcIndex(i.flightplan))), m_sequence_OMDW.end());
			continue;
		}


		//if any aircraft takes off remove it automatically
		auto fp = i.flightplan;
		EuroScopePlugIn::CRadarTarget rt = fp.GetCorrelatedRadarTarget();
		auto cad = fp.GetFlightPlanData();
		std::string remarks = cad.GetRemarks();
		const char* test = NULL;
		test = strstr(remarks.c_str(), "/CTOT");
		if (rt.GetGS() > 80 || !test)
		{
			remarks = std::regex_replace(remarks, std::regex("\\/CTOT"), "");
			cad.SetRemarks(remarks.c_str());
			cad.AmendFlightPlan();
			m_TOSequenceList.RemoveFpFromTheList(fp);
			m_sequence_OMDW.erase(std::remove(m_sequence_OMDW.begin(), m_sequence_OMDW.end(), m_sequence_OMDW.at(_SelectAcIndex(fp))), m_sequence_OMDW.end());
		}
	}
	std::sort(m_sequence_OMDW.begin(), m_sequence_OMDW.end());
	for (CTOTData &i : m_sequence_OMDW)
	{
		//get the number in sequence for everyone
		i.sequence = _SelectAcIndex(i.flightplan) + 1;
	}
	if (m_sequence_OMDW.empty())
	{
		return;
	}
	recalculateCTOT(*m_sequence_OMDW.begin());
}
void CUAEController::updateListOMAA()
{
	//some housekeeping 
	for (CTOTData i : m_sequence_OMAA)
	{
		//add all aircraft in sequence to the euroscope list
		m_TOSequenceList.AddFpToTheList(i.flightplan);
	}
	for (CTOTData i : m_sequence_OMAA)
	{
		EuroScopePlugIn::CFlightPlan first = FlightPlanSelectFirst();
		EuroScopePlugIn::CFlightPlan temp = FlightPlanSelectNext(first);
		bool found = false;
		if (first.GetCallsign() == i.flightplan.GetCallsign()) found = true;
		while (temp.IsValid() && !found)
		{
			if (temp.GetCallsign() == i.flightplan.GetCallsign())
			{
				found = true;
				break;
			}
			temp = FlightPlanSelectNext(temp);
		}
		if (!found)
		{
			m_TOSequenceList.RemoveFpFromTheList(i.flightplan);
			m_sequence_OMAA.erase(std::remove(m_sequence_OMAA.begin(), m_sequence_OMAA.end(), m_sequence_OMAA.at(_SelectAcIndex(i.flightplan))), m_sequence_OMAA.end());
			continue;
		}


		//if any aircraft takes off remove it automatically
		auto fp = i.flightplan;
		EuroScopePlugIn::CRadarTarget rt = fp.GetCorrelatedRadarTarget();
		auto cad = fp.GetFlightPlanData();
		std::string remarks = cad.GetRemarks();
		const char* test = NULL;
		test = strstr(remarks.c_str(), "/CTOT");
		if (rt.GetGS() > 80 || !test)
		{
			remarks = std::regex_replace(remarks, std::regex("\\/CTOT"), "");
			cad.SetRemarks(remarks.c_str());
			cad.AmendFlightPlan();
			m_TOSequenceList.RemoveFpFromTheList(fp);
			m_sequence_OMAA.erase(std::remove(m_sequence_OMAA.begin(), m_sequence_OMAA.end(), m_sequence_OMAA.at(_SelectAcIndex(fp))), m_sequence_OMAA.end());
		}
	}
	std::sort(m_sequence_OMAA.begin(), m_sequence_OMAA.end());
	for (CTOTData &i : m_sequence_OMAA)
	{
		//get the number in sequence for everyone
		i.sequence = _SelectAcIndex(i.flightplan) + 1;
	}
	if (m_sequence_OMAA.empty())
	{
		return;
	}
	recalculateCTOT(*m_sequence_OMAA.begin());
}
void CUAEController::recalculateCTOT(CTOTData inserted)
//recalculates the CTOT for each flightplan in sequence following and inculding "inserted"
{
	std::string dep = inserted.flightplan.GetFlightPlanData().GetOrigin();
	//get the number in sequence for our inserted fp
	if (dep == "OMDB")
	{
		auto pos = std::find(m_sequence_OMDB.begin(), m_sequence_OMDB.end(), inserted);
		//safeguard if not found due to whatever 
		if (pos == m_sequence_OMDB.end())
		{
			return;
		}
		//iterate over the sequence starting from "inserted"
		for (auto &it = pos; it != m_sequence_OMDB.end() - 1; it++)
		{
			CTime curr = CTime::GetCurrentTime();
			CTOTData temp1 = *it;
			CString test = temp1.flightplan.GetCallsign();
			CTOTData &temp2 = *(it + 1);
			CString test2 = temp2.flightplan.GetCallsign();
			if (temp2.manual) continue;
			CTimeSpan inc = getIncrement(temp1.flightplan, temp2.flightplan);
			temp2.CTOT = std::max(temp2.CTOT, temp1.CTOT + inc);
			temp2.TOBT = temp2.CTOT - taxitime;
		}
	}
	if (dep == "OMSJ")
	{
		auto pos = std::find(m_sequence_OMSJ.begin(), m_sequence_OMSJ.end(), inserted);
		//safeguard if not found due to whatever 
		if (pos == m_sequence_OMSJ.end())
		{
			return;
		}
		//iterate over the sequence starting from "inserted"
		for (auto &it = pos; it != m_sequence_OMSJ.end() - 1; it++)
		{
			CTime curr = CTime::GetCurrentTime();
			CTOTData temp1 = *it;
			CString test = temp1.flightplan.GetCallsign();
			CTOTData &temp2 = *(it + 1);
			CString test2 = temp2.flightplan.GetCallsign();
			if (temp2.manual) continue;
			CTimeSpan inc = getIncrement(temp1.flightplan, temp2.flightplan);
			temp2.CTOT = std::max(temp2.CTOT, temp1.CTOT + inc);
			temp2.TOBT = temp2.CTOT - taxitime;
		}
	}
	if (dep == "OMDW")
	{
		auto pos = std::find(m_sequence_OMDW.begin(), m_sequence_OMDW.end(), inserted);
		//safeguard if not found due to whatever 
		if (pos == m_sequence_OMDW.end())
		{
			return;
		}
		//iterate over the sequence starting from "inserted"
		for (auto &it = pos; it != m_sequence_OMDW.end() - 1; it++)
		{
			CTime curr = CTime::GetCurrentTime();
			CTOTData temp1 = *it;
			CString test = temp1.flightplan.GetCallsign();
			CTOTData &temp2 = *(it + 1);
			CString test2 = temp2.flightplan.GetCallsign();
			if (temp2.manual) continue;
			CTimeSpan inc = getIncrement(temp1.flightplan, temp2.flightplan);
			temp2.CTOT = std::max(temp2.CTOT, temp1.CTOT + inc);
			temp2.TOBT = temp2.CTOT - taxitime;
		}
	}
	if (dep == "OMAA")
	{
		auto pos = std::find(m_sequence_OMAA.begin(), m_sequence_OMAA.end(), inserted);
		//safeguard if not found due to whatever 
		if (pos == m_sequence_OMAA.end())
		{
			return;
		}
		//iterate over the sequence starting from "inserted"
		for (auto &it = pos; it != m_sequence_OMAA.end() - 1; it++)
		{
			CTime curr = CTime::GetCurrentTime();
			CTOTData temp1 = *it;
			CString test = temp1.flightplan.GetCallsign();
			CTOTData &temp2 = *(it + 1);
			CString test2 = temp2.flightplan.GetCallsign();
			if (temp2.manual) continue;
			CTimeSpan inc = getIncrement(temp1.flightplan, temp2.flightplan);
			temp2.CTOT = std::max(temp2.CTOT, temp1.CTOT + inc);
			temp2.TOBT = temp2.CTOT - taxitime;
		}
	}
}
CTimeSpan CUAEController::getIncrement(EuroScopePlugIn::CFlightPlan fp1, EuroScopePlugIn::CFlightPlan fp2)
//returns the required difference in CTOT when fp2 is trailing fp1
{
	CTimeSpan increment;
	CString sid1 = fp1.GetFlightPlanData().GetSidName();
	CString sid2 = fp2.GetFlightPlanData().GetSidName();

	char wtc1 = fp1.GetFlightPlanData().GetAircraftWtc();
	char wtc2 = fp2.GetFlightPlanData().GetAircraftWtc();
	switch (wtc1)
	{
	case 'J':
	{
		if (wtc2 == 'H')
		{
			increment = CTimeSpan(0, 0, 2, 0);
		}
		if (wtc2 == 'M' || wtc2 == 'L')
		{
			increment = CTimeSpan(0, 0, 3, 0);
		}
		break;
	}
	case 'H':
	{
		if (wtc2 == 'M' || wtc2 == 'L')
		{
			increment = CTimeSpan(0, 0, 2, 0);
		}
		else {

		}
		break;
	}
	case 'M':
	{
		if (wtc2 == 'L')
		{
			increment = CTimeSpan(0, 0, 2, 0);
		}
		break;
	}
	}
	if (increment == NULL)
	{
		if (sid1 != sid2)
		{
			//RRSM
			increment = CTimeSpan(0, 0, 1, 0);
		}
		else
			increment = CTimeSpan(0, 0, 2, 0);
	}
	if (sid1 == sid2 && (strstr(sid1, "ANVIX") || strstr(sid1, "IVURO")))
		increment = std::max(increment, CTimeSpan(0, 0, 3, 0));

	return increment;
}
int CUAEController::_SelectAcIndex(EuroScopePlugIn::CFlightPlan flightplan)
{
	// helper function to search our sequence for "flightplan"
	int i = 0;
	CString temp2 = flightplan.GetCallsign();

	if (m_sequence_OMDB.empty() && m_sequence_OMSJ.empty() && m_sequence_OMDW.empty() && m_sequence_OMAA.empty())
	{
		return -1;
	}
	for (CTOTData test : m_sequence_OMDB)
	{
		try {
			CString temp1 = test.flightplan.GetCallsign();
			if (temp1.Compare(temp2) == 0)
			{
				return  i;
			}
			i++;
		}
		catch (const std::exception) { continue; }
	}
	i = 0;
	for (CTOTData test : m_sequence_OMSJ)
	{
		try {
			CString temp1 = test.flightplan.GetCallsign();
			if (temp1.Compare(temp2) == 0)
			{
				return  i;
			}
			i++;
		}
		catch (const std::exception) { continue; }
	}
	i = 0;
	for (CTOTData test : m_sequence_OMDW)
	{
		try {
			CString temp1 = test.flightplan.GetCallsign();
			if (temp1.Compare(temp2) == 0)
			{
				return  i;
			}
			i++;
		}
		catch (const std::exception) { continue; }
	}
	i = 0;
	for (CTOTData test : m_sequence_OMAA)
	{
		try {
			CString temp1 = test.flightplan.GetCallsign();
			if (temp1.Compare(temp2) == 0)
			{
				return  i;
			}
			i++;
		}
		catch (const std::exception) { continue; }
	}
	return -1;
}
void CUAEController::cleanupStands()
{
	int i = 0;
	int j = 0;
	for (auto airport : activeAirports)
	{
		std::regex re, re1;
		if (airport == "OMDB")
		{
			re = std::regex(R"(\/STAND([A-Z]\d{1,2}))");
			re1 = re;
		}
		if (airport == "OMSJ")
		{
			re = std::regex(R"(\/STAND(\d{1,2}[A-Z]?))");
			re1 = re;
		}
		if (airport == "OMAA")
		{
			re = std::regex(R"(\/STAND(\d{1,3}))");
			re1 = std::regex(R"(\/STAND(GA))");
		}
		for (auto& it : data.at(airport))
		{

			if (!it.second.isEmpty)
			{
				auto first = RadarTargetSelectFirst();
				if (first.GetPosition().GetPosition().DistanceTo(it.second.position) < TOL)
					goto outer2;
				auto temp = RadarTargetSelectNext(first);
				while (temp.IsValid())
				{
					if (temp.GetPosition().GetPosition().DistanceTo(it.second.position) < TOL)
					{
						goto outer2;
					}

					temp = RadarTargetSelectNext(temp);
				}
				i++;
				it.second.isEmpty = true;
				std::string logstring = "Stand ";
				logstring += it.second.number;
				logstring += " is now emtpy again.";
				LOG_F(INFO, logstring.c_str());
			}
			if (it.second.isAssigned)
			{
				auto first = FlightPlanSelectFirst();
				
				std::string remarks = first.GetFlightPlanData().GetRemarks();
				std::smatch match,match1;
				if (std::regex_search(remarks, match, re) || std::regex_search(remarks, match1, re1))
				{
					if (it.second.number == match.str(1) || it.second.number == match1.str(1))
						goto outer2;
				}
				auto temp = FlightPlanSelectNext(first);
				while (temp.IsValid())
				{
					remarks = temp.GetFlightPlanData().GetRemarks();
					if (std::regex_search(remarks, match, re) || std::regex_search(remarks, match1, re1))
					{
						if (it.second.number == match.str(1) || it.second.number == match1.str(1))
							goto outer2;
					}
					temp = FlightPlanSelectNext(temp);
				}
				it.second.isAssigned = false;
				j++;
				std::string logstring = "Stand ";
				logstring += it.second.number;
				logstring += " no longer assigned. Removing it...";
				LOG_F(INFO, logstring.c_str());
				for (auto &temp : standmapping.at(airport))
				{
					if (temp.second.number == it.second.number)
						standmapping.at(airport).erase(temp.first);
				}
				std::string code = it.second.mAirlinecode;
				if (code == "UAE")
				{
					for (auto &temp : standsUAE.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "ETD")
				{
					for (auto &temp : standsETD.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "PAX" || code == "ABY")
				{
					for (auto &temp : standsPAX.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "ABY" || code == "PAX")
				{
					for (auto &temp : standsABY.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "CARGO1")
				{
					for (auto &temp : standsCargoSpecial.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}

				if (code == "CARGO" || code == "CLC")
				{
					for (auto &temp : standsCARGO.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}

				if (code == "LWC" || code == "CLC")

				{
					for (auto &temp : standsLOWCOST.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "GA")
				{
					for (auto &temp : standsGA.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "VIP")
				{
					for (auto &temp : standsVIP.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}
				if (code == "ALL")
				{
					for (auto &temp : standsOverflow.at(airport))
					{
						if (temp.number == it.second.number)
							temp.isAssigned = false;
					}
				}

			}
		outer2:
			continue;
		}
		
	}

	std::string logstring = "CleanupStands finished with: ";
	logstring += std::to_string(i);
	logstring += " new empty stands and ";
	logstring += std::to_string(j);
	logstring += " no longer assigned stands";
	LOG_F(INFO, logstring.c_str());
	return;
}
Stand CUAEController::extractRandomStand(std::vector<Stand> stands, char size, std::string icao)
{
	LOG_F(INFO, "Extracting stand...");
	LOG_F(INFO, "Following stands are set as occupied or already assigned: ");
	std::string logstring;
	for (auto element : stands)
	{
		if (element.isAssigned || !element.isEmpty)
		{
			logstring += element.number;
			logstring += " ";
		}
	}
	LOG_F(INFO, logstring.c_str());
	std::shuffle(std::begin(stands), std::end(stands), std::random_device());
	for (auto stand : stands)
	{
		if (!stand.isAssigned && stand.isEmpty && stand.isInFlytampa && stand.mSize >= size)
		{
			return stand;
		}

	}
	std::shuffle(std::begin(standsOverflow.at(icao)), std::end(standsOverflow.at(icao)), std::random_device());
	for (auto stand : standsOverflow.at(icao))
	{
		if (!stand.isAssigned && stand.isEmpty && stand.isInFlytampa && stand.mSize >= size)
		{
			return stand;
		}
	}
	DisplayUserMessage("StandAssigner", icao.c_str(), "Error assigning stand", true, true, true, true, true);
	auto errorval = standsOverflow.at("OMDB").at(0);
	errorval.number = "Z00";
	return errorval;
}
char CUAEController::determineAircraftCat(EuroScopePlugIn::CFlightPlan fp)
{
	char wtc = fp.GetFlightPlanData().GetAircraftWtc();
	std::string actype = fp.GetFlightPlanData().GetAircraftFPType();
	auto found = parkingdict.find(actype);
	if (found != parkingdict.end())
		return found->second.c_str()[0];
	else {
		if (wtc == 'L')
			return 'B';
		if (wtc == 'J')
			return 'F';
		if (wtc == 'H')
			return 'E';
		if (wtc == 'M')
			return 'C';
	}
}
void CUAEController::readStandFile(std::string dir, std::string airport)
{
	io::CSVReader<8, io::trim_chars<' '>, io::no_quote_escape<','>> in(dir);
	in.read_header(io::ignore_extra_column, "Standnumber", "latitude", "longitude", "airlinecode", "neighbor1", "neighbor2", "size", "flytampa");
	std::string StandNumber, lat, Long, airlinecode, neighbor1, neighbor2, size, flytampa;
	std::unordered_map<std::string, Stand> thisdata;
	while (in.read_row(StandNumber, lat, Long, airlinecode, neighbor1, neighbor2, size, flytampa))
	{
		Stand temp = Stand(StandNumber, lat, Long, airlinecode, neighbor1, neighbor2, size, flytampa, airport);
		std::pair<std::string, Stand> temp2(StandNumber, temp);
		thisdata.insert(temp2);
	}
	std::pair<std::string, std::unordered_map<std::string, Stand>> temp3(airport, thisdata);
	data.insert(temp3);
	std::string logstring;
	logstring = "Stand file read for ";
	logstring += airport;
	LOG_F(INFO, logstring.c_str());
}
void CUAEController::readCallsignFile(std::string dir, std::string airport)
{
	//reading callsign mapping
	io::CSVReader<2, io::trim_chars<' '>, io::no_quote_escape<','>> in(dir);
	in.read_header(io::ignore_extra_column, "Callsign", "ToAssign");
	std::string Callsign, ToAssign;
	std::unordered_map<std::string, std::string> thiscallsignmap;
	while (in.read_row(Callsign, ToAssign))
	{
		std::pair<std::string, std::string> temp3(Callsign, ToAssign);
		thiscallsignmap.insert(temp3);
	}
	std::pair<std::string, std::unordered_map<std::string, std::string>> temp(airport, thiscallsignmap);
	callsignmap.insert(temp);
	std::string logstring;
	logstring = "Callsignmapping file read for ";
	logstring += airport;
	LOG_F(INFO, logstring.c_str());
}
std::string CUAEController::getRouteRegion(std::unordered_map<std::string,RouteData> routedata,std::string icaodep, std::string icaodest)
{
	std::string returnvalue;
	if (routedata.find(icaodep) == routedata.end())
	{
		
		returnvalue =  "?";
	}


	bool foundRoute = false;
	for (auto temp : routedata.at(icaodep).icaos)
	{
		if (temp == icaodest)
		{
			foundRoute = true;
			returnvalue = icaodest;
			break;
		}

	}
	if (!foundRoute)
	{
		for (auto temp : routedata.at(icaodep).icaos)
		{
			if (temp == icaodest.substr(0, 3))
			{

				returnvalue = icaodest.substr(0, 3);
				foundRoute = true;
				break;
			}
		}
		if (!foundRoute)
		{
			for (auto temp : routedata.at(icaodep).icaos)
			{
				if (temp == icaodest.substr(0, 2))
				{

					returnvalue = icaodest.substr(0, 2);
					foundRoute = true;
					break;
				}
			}
			if (!foundRoute)
			{
				for (auto temp : routedata.at(icaodep).icaos)
				{
					if (temp == icaodest.substr(0, 1))
					{
						returnvalue = icaodest.substr(0, 1);

						foundRoute = true;
						break;
					}
				}
			}
			if (!foundRoute)
			{
				returnvalue = "?";
				std::string logstring = "No mandatory (dummy) route to " + icaodest + " found.";
				LOG_F(INFO, logstring.c_str());
				return returnvalue;
			}

		}
	}
	return returnvalue;
}