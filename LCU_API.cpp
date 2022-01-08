#include "LCU_API.h"
#include "atlbase.h"
#include "atlstr.h"
#include "Utils.h"
#include <string>
#include "json/json.h"



using namespace LCUAPI;
using namespace web;
using namespace web::websockets::client;

LCU_API::~LCU_API() {
	if (ws_client) {
		delete ws_client;
	}
}

LCU_API::LCU_API(EventHandleType event_handle_type):event_handle_type(event_handle_type) {
	connected = false;
	host = "127.0.0.1";
	prot[0] = "https://";
	prot[1] = "wss://";
}

bool LCU_API::BindEvent(const std::string& method, EVENT_CALLBACK call_back, bool force) {
	if (event_handle_type != EventHandleType::BIND) {
		std::cout << "event_handle_type is not bind" << std::endl;
		return false;
	}
	bind[method] = call_back;
	if (!force) {
		if (!IsAPIServerConnected()) {
			return false;
		}
	}
	
	websocket_outgoing_message msg;
	char msg_buffer[128];
	sprintf_s(msg_buffer, 128, "[5, \"%s\"]", method.data());
	msg.set_utf8_message(msg_buffer);
	auto sub_ret = ws_client->send(msg).then([method]() {
		std::cout << "subscrib " << method <<std::endl;
	});
	sub_ret.wait();
	
	return true;
}

bool LCU_API::UnBindEvent(const std::string& method) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	auto index = bind.find(method);
	if (index == bind.end()) {
		return true;
	}
	websocket_outgoing_message msg;
	char msg_buffer[128];
	sprintf_s(msg_buffer, 128, "[6, \"%s\"]", method.data());
	msg.set_utf8_message(msg_buffer);
	auto sub_ret = ws_client->send(msg).then([&method]() {
		std::cout << "unsubscrib " << method << std::endl;
		});
	sub_ret.wait();
	bind.erase(index);
	return true;
}

bool LCU_API::AddEventFilter(const std::string &uri, EventType type, EVENT_CALLBACK call_back) {
	if (event_handle_type != EventHandleType::FILTER) {
		std::cout << "event_handle_type is not filter" << std::endl;
		return false;
	}
	switch (type)
	{
	case EventType::CREATE:
		filter[uri][0] = call_back;
		break;
	case EventType::UPDATE:
		filter[uri][1] = call_back;
		break;
	case EventType::DEL:
		filter[uri][2] = call_back;
		break;
	default:
		return false;
	}
	return true;
}

bool LCU_API::RemoveEventFilter(const std::string &uri, EventType type) {
	if (filter.find(uri) == filter.end()) {
		return true;
	}
	switch (type)
	{
	case EventType::CREATE:
		filter.at(uri)[0] = nullptr;
		break;
	case EventType::UPDATE:
		filter.at(uri)[1] = nullptr;
		break;
	case EventType::DEL:
		filter.at(uri)[2] = nullptr;
		break;
	case EventType::ALL:
		filter.at(uri)[0] = nullptr;
		filter.at(uri)[1] = nullptr;
		filter.at(uri)[2] = nullptr;
		break;
	default:
		return false;
		break;
	}
	if (!filter.at(uri)[0] &&
		!filter.at(uri)[0] &&
		!filter.at(uri)[0]) {
		filter.erase(filter.find(uri));
	}
	return true;
}
// 连接LCU
bool LCU_API::Connect() {
	connected = false;
	try {
		if (!auth.GetLeagueClientInfo()) {
			return connected;
		}

		auto request_call = &LCU_API::Request;
		auto obj = this;
		GET = std::bind(request_call, obj, "GET", std::placeholders::_1,
			std::placeholders::_2, auth.leagueHeader, "", "", auth.leaguePort);
		POST = std::bind(request_call, obj, "POST", std::placeholders::_1,
			std::placeholders::_2, auth.leagueHeader, "", "", auth.leaguePort);
		PUT = std::bind(request_call, obj, "PUT", std::placeholders::_1,
			std::placeholders::_2, auth.leagueHeader, "", "", auth.leaguePort);
		DELETe = std::bind(request_call, obj, "DELETE", std::placeholders::_1,
			std::placeholders::_2, auth.leagueHeader, "", "", auth.leaguePort);
		PATCH = std::bind(request_call, obj, "PATCH", std::placeholders::_1,
			std::placeholders::_2, auth.leagueHeader, "", "", auth.leaguePort);

		// check lol client is ready
		std::string load_ok_str = GET(url("/lol-clash/v1/ping"), "");
		if (load_ok_str != R"("pong")") {
			return connected;
		}

	}
	catch (...) {
		return connected;
	}
	
	// clear old wss_client
	if (ws_client) {
		delete ws_client;
		ws_client = nullptr;
	}
	// create a new wss_client
	websocket_client_config ws_config;
	ws_config.set_validate_certificates(false);
	std::string auth_str = "Basic ";
	auth_str += auth.leagueToken;
	ws_config.headers()[L"Authorization"] = CA2W(auth_str.c_str());

	ws_client = new websocket_callback_client(ws_config);

	std::string ws_str = prot[1] + host + ":" + std::to_string(auth.leaguePort);
	wchar_t* ws_wstr = CA2W(ws_str.c_str());
	try {
		// connect with lcu
		auto con_ret = ws_client->connect(ws_wstr).then([this]() {
			connected = true;
			std::cout << "wss connected" << std::endl;
		});
		con_ret.wait();

		ws_client->set_message_handler([this](websocket_incoming_message msg) {
			this->HandleEventMessage(msg);
			});
		ws_client->set_close_handler([this](websocket_close_status close_status, const utility::string_t& reason, const std::error_code& error) {
			this->connected = false;
			std::cout << "wss connect closed" << std::endl;
			});

		if (event_handle_type == EventHandleType::FILTER) {
			websocket_outgoing_message msg;
			msg.set_utf8_message("[5, \"OnJsonApiEvent\"]");
			auto sub_ret = ws_client->send(msg).then([]() {
				std::cout << "subscribed all events" << std::endl;
				});
			sub_ret.wait();
		}
		else if (event_handle_type == EventHandleType::BIND) { // rebind events
			for (auto item : bind) {
				BindEvent(item.first, item.second,true);
			}
		}
	}
	catch (websocket_exception const& ex) {
		std::cout << "wss error:" << ex.what() << std::endl;
		return connected;
	}
	return connected;
}

void LCU_API::HandleEventMessage(websocket_incoming_message& msg) {
	if (!msg.length()) {
		return;
	}
	bool need_handle = false;
	std::string msg_str = msg.extract_string().get();
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(msg_str.c_str(), msg_str.c_str() + static_cast<int>(msg_str.length() - 1), &root, &err)) {
		if (root[0].asInt() == 8) {
			if (event_handle_type == EventHandleType::FILTER) {
				std::string uri = root[2]["uri"].asString();
				if (filter.find(uri) != filter.end()) {
					int type_index = -1;
					std::string event_type = root[2]["eventType"].asString();
					if (!event_type.compare("Create")) {
						type_index = 0;
					}
					else if (!event_type.compare("Update")) {
						type_index = 1;
					}
					else if (!event_type.compare("Delete")) {
						type_index = 2;
					}
					if (type_index != -1) {
						if (filter.at(uri)[type_index]) {
							need_handle = true;
							filter.at(uri)[type_index](root);
						}
					}
				}
			}
			else if (event_handle_type == EventHandleType::BIND) {
				std::string method = root[1].asString();
				if (bind.find(method) != bind.end()) {
					need_handle = true;
					bind.at(method)(root);
				}
			}
		}
	}
#ifndef  LOG_ALL_EVENT
	if (!need_handle) {
		return;
	}
#endif // ! LOG_ALL_EVENT

#ifdef USE_LOG
#ifdef JSON_FORMAT
	string log_content = utils->formatJson(msg_str);
#else
	std::string log_content = msg_str;
#endif // JSON_FORMAT
#ifdef USE_GBK
	log_content = utils->UtfToGbk(log_content.c_str());
#endif // USE_GBK
	std::cout << "==================EVENT INFO================" << std::endl;
	std::cout << log_content << std::endl;
#endif // USE_LOG
	
}

bool LCU_API::IsAPIServerConnected() {
	if (!connected) {
		std::cout << "lcu not connected" << std::endl;
	}
	return connected;
}

std::string LCU_API::url(const std::string &api) {
	return prot[0] + host + api;
}

std::string LCU_API::Request(const std::string& method, const std::string& url, const std::string& requestData, const std::string& header,
	const std::string& cookies, const std::string& returnCookies, int port) {
	std::string result = http_client.Request(method, url, requestData, header, cookies, returnCookies, port);
#ifdef  USE_LOG
	std::cout << "==================" << method << " INFO================" << std::endl;
	std::cout << "API:" << url << std::endl;
	if (!requestData.empty()) {
		std::string content;
#ifdef JSON_FORMAT
		content = utils->formatJson(requestData);
#else
		content = requestData;
#endif // JSON_FORMAT
#ifdef USE_GBK
		content = utils->UtfToGbk(content.c_str());
#endif // USE_GBK
		std::cout << "Request Data:" << std::endl << content << std::endl;
	}
	if (!result.empty()) {
		std::string content;
#ifdef JSON_FORMAT
		content = utils->formatJson(result);
#else
		content = result;
#endif // JSON_FORMAT
#ifdef USE_GBK
		content = utils->UtfToGbk(content.c_str());
#endif // USE_GBK
		std::cout << "Result:" << std::endl << content << std::endl;
	}
#endif //  USE_LOG
	return result;
}

template<typename T>
bool LCU_API::ResultCheck(const std::string& data, const std::string& attr, T aim) {
	bool ret = false;
	if (!data.empty())
	{
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		JSONCPP_STRING err;
		Json::Value root;
		if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
			try
			{
				if (root[attr].as<T>() == aim) {
					ret = true;
				}
			}
			catch (const std::exception&)
			{
				ret = false;
			}
		}
	}
	return ret;
}

// ==================EVENT==================
bool LCU_API::BindEventAuto(const std::string& event_name,const std::string& uri) {
	if (binded_event.find(event_name) == binded_event.end()) {
		binded_event[event_name][0] = nullptr;
		binded_event[event_name][1] = nullptr;
		binded_event[event_name][2] = nullptr;
	}
	else {
		return true;
	}
	return BindEvent(event_name, [this,event_name,uri](Json::Value& data) {
		if (data[2]["uri"] == uri) {
			if (data[2]["eventType"] == "Create") {
				if (this->binded_event[event_name][0]) {
					this->binded_event[event_name][0](data);
				}
			}
			else if (data[2]["eventType"] == "Update") {
				if (this->binded_event[event_name][1]) {
					this->binded_event[event_name][1](data);
				}
			}
			else if (data[2]["eventType"] == "Delete") {
				if (this->binded_event[event_name][2]) {
					this->binded_event[event_name][2](data);
				}
			}
		}
	});
}

bool LCU_API::OnCreateSearch(EVENT_CALLBACK call_back) {
	std::string event_name = "OnJsonApiEvent_lol-matchmaking_v1_search";
	bool ret = BindEventAuto(event_name, "/lol-matchmaking/v1/search");
	binded_event[event_name][0] = call_back;
	return ret;
}
bool LCU_API::OnUpdateSearch(EVENT_CALLBACK call_back) {
	std::string event_name = "OnJsonApiEvent_lol-matchmaking_v1_search";
	bool ret = BindEventAuto(event_name, "/lol-matchmaking/v1/search");
	binded_event[event_name][1] = call_back;
	return ret;
}
bool LCU_API::OnDeleteSearch(EVENT_CALLBACK call_back) {
	std::string event_name = "OnJsonApiEvent_lol-matchmaking_v1_search";
	bool ret = BindEventAuto(event_name, "/lol-matchmaking/v1/search");
	binded_event[event_name][2] = call_back;
	return ret;
}

bool LCU_API::OnCreateRoom(EVENT_CALLBACK call_back) {
	std::string event_name = "OnJsonApiEvent_lol-lobby_v2_lobby";
	bool ret = BindEventAuto(event_name, "/lol-lobby/v2/lobby");
	binded_event[event_name][0] = call_back;
	return ret;
}

bool LCU_API::OnCloseRoom(EVENT_CALLBACK call_back) {
	std::string event_name = "OnJsonApiEvent_lol-lobby_v2_lobby";
	bool ret = BindEventAuto(event_name, "/lol-lobby/v2/lobby");
	binded_event[event_name][2] = call_back;
	return ret;
}

bool LCU_API::OnUpdateRoom(EVENT_CALLBACK call_back) {
	std::string event_name = "OnJsonApiEvent_lol-lobby_v2_lobby";
	bool ret = BindEventAuto(event_name, "/lol-lobby/v2/lobby");
	binded_event[event_name][1] = call_back;
	return ret;
}

// ==================API==================
bool LCU_API::BuildRoom(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string body = R"({"queueId":)" + std::to_string(static_cast<int>(type)) + "}";
	return ResultCheck(POST(url("/lol-lobby/v2/lobby"), body), "errorCode", NULL);
}

bool LCU_API::BuildTFTNormalRoom() {
	return BuildRoom(QueueID::TFTNormal);
}

bool LCU_API::BuildTFTRankRoom() {
	return BuildRoom(QueueID::TFTRanked);
}

bool LCU_API::ExitRoom() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	DELETe(url("/lol-lobby/v2/lobby"), "");
	return true;
}

Json::Value LCU_API::GetRoom() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-lobby/v2/lobby"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		return root;
	}
	return NULL;
}

bool LCU_API::OpenTeam() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret = PUT(url("/lol-lobby/v2/lobby/partyType"), R"("open")");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

bool LCU_API::CloseTeam() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret = PUT(url("/lol-lobby/v2/lobby/partyType"), R"("closed")");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

bool LCU_API::LobbyAvailability() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret = GET(url("/lol-lobby/v1/lobby/availability"), "");
	if (ret == R"("Available")") {
		return true;
	}
	else {
		return false;
	}
}

Json::Value LCU_API::GetInvitations() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-lobby/v1/lobby/invitations"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		return root;
	}
	return NULL;
}

Json::Value LCU_API::GetGameMode() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-lobby/v1/parties/gamemode"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return NULL;
}

bool LCU_API::SetMetaData(PositionPref first, PositionPref second) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* types[] = { "TOP", "JUNGLE", "MIDDLE", "BOTTOM", "UTILITY", "FILL", "UNSELECTED" };
	char const* format = R"({"championSelection" : null,"positionPref" :["%s","%s"] ,"properties" : null,"skinSelection" : null})";
	char data[128];
	sprintf_s(data, 128, format, types[static_cast<int>(first)], types[static_cast<int>(second)]);
	std::string ret = PUT(url("/lol-lobby/v1/parties/metadata"), data);
	return true;
}

Json::Value LCU_API::GetPartiedPlayer() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-lobby/v1/parties/player"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return NULL;
}

bool LCU_API::SetQueue(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	PUT(url("/lol-lobby/v1/parties/queue"), std::to_string(static_cast<int>(type)));
	return true;
}

bool LCU_API::GiveLeaderRole(const char* party_id, const char* puuid) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v1/parties/%s/members/%s/role";
	char url_data[128];
	sprintf_s(url_data, 128, format, party_id, puuid);
	std::string ret = PUT(url(url_data), "LEADER");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

Json::Value LCU_API::GetCommsMembers() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-lobby/v2/comms/members"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return NULL;
}

std::string LCU_API::GetCommsToken() {
	if (!IsAPIServerConnected()) {
		return "";
	}
	return GET(url("/lol-lobby/v2/comms/token"), "");
}

bool LCU_API::CheckPartyEligibility(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string data = POST(url("/lol-lobby/v2/eligibility/party"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			// 解析
			for (auto item : root) {
				if (item["queueId"].asInt() == static_cast<int>(type)) {
					return item["eligible"].asBool();
				}
			}
		}
	}
	return false;
}

bool LCU_API::CheckSelfEligibility(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string data = POST(url("/lol-lobby/v2/eligibility/self"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			// 解析
			for (auto item : root) {
				if (item["queueId"].asInt() == static_cast<int>(type)) {
					return item["eligible"].asBool();
				}
			}
		}
	}
	return false;
}

Json::Value LCU_API::GetLobbyInvitations() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-lobby/v2/lobby/invitations"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
			if (root.isArray()) {
				return root;
			}
		}
	}
	return NULL;
}

bool LCU_API::InviteBySummonerIdS(std::vector<long long> ids) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format_out = "[]";
	const char* format_in = R"({"toSummonerId":%u},)";
	char row_data[32];
	std::string data = "[";
	for (long long item : ids) {
		sprintf_s(row_data, 32, format_in, item);
		data += row_data;
	}
	data = data.substr(0, data.length() - 1);
	data += "]";
	std::string ret = POST(url("/lol-lobby/v2/lobby/invitations"), data);
	if (ret.find("[") != -1) {
		return true;
	}
	return false;
}


bool LCU_API::StartQueue() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	if (POST(url("/lol-lobby/v2/lobby/matchmaking/search"), "").empty()) {
		return true;
	}
	else {
		return false;
	}
}

bool LCU_API::StopQueue() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	if (DELETe(url("/lol-lobby/v2/lobby/matchmaking/search"), "").empty()) {
		return true;
	}
	else{
		return false;
	}
}

SearchState LCU_API::GetSearchState() {
	if (!IsAPIServerConnected()) {
		return SearchState::Invalid;
	}
	std::string data = GET(url("/lol-lobby/v2/lobby/matchmaking/search-state"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			std::string state = root["searchState"].asString();

			if (state == "Invalid") {
				return SearchState::Invalid;
			}
			else if (state == "Searching") {
				return SearchState::Searching;
			}
			else if (state == "Found") {
				return SearchState::Found;
			}
		}
	}
	return SearchState::Invalid;
}

Json::Value LCU_API::GetLobbyMembers() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-lobby/v2/lobby/members"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			return root;
		}
	}
	return NULL;
}

bool LCU_API::SetPositionPreferences(PositionPref first, PositionPref second) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* types[] = { "TOP", "JUNGLE", "MIDDLE", "BOTTOM", "UTILITY", "FILL", "UNSELECTED" };
	const char* format = R"({"firstPreference": "%s","secondPreference": "%s"})";
	char data[128];
	sprintf_s(data, 128, format, types[static_cast<int>(first)], types[static_cast<int>(second)]);
	std::string ret = PUT(url("/lol-lobby/v2/lobby/members/localMember/position-preferences"), data);
	if (ret.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::GrantInviteBySummonerId(long long id) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/lobby/members/%u/grant-invite";
	char url_buffer[64];
	sprintf_s(url_buffer, 64, format, id);
	std::string ret = POST(url(url_buffer), "");
	if (ret == std::to_string(id)) {
		return true;
	}
	return false;
}

bool LCU_API::KickSummonerBySummonerId(long long id) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/lobby/members/%u/kick";
	char url_buffer[64];
	sprintf_s(url_buffer, 64, format, id);
	std::string ret = POST(url(url_buffer), "");
	if (ret == std::to_string(id)) {
		return true;
	}
	return false;
}

bool LCU_API::PromoteSummonerBySummonerId(long long id) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/lobby/members/%u/promote";
	char url_buffer[64];
	sprintf_s(url_buffer, 64, format, id);
	std::string ret = POST(url(url_buffer), "");
	if (ret == std::to_string(id)) {
		return true;
	}
	return false;
}

bool LCU_API::RevokeInviteBySummonerId(long long id) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/lobby/members/%u/revoke-invite";
	char url_buffer[64];
	sprintf_s(url_buffer, 64, format, id);
	std::string ret = POST(url(url_buffer), "");
	if (ret == std::to_string(id)) {
		return true;
	}
	return false;
}

bool LCU_API::SetPartyType(PartyType type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* types[] = {"open", "closed"};
	std::string ret = PUT(url("/lol-lobby/v2/lobby/partyType"), types[static_cast<int>(type)]);
	if (ret.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::IsPartyActive() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret = GET(url("/lol-lobby/v2/party-active"), "");
	if (ret == "true") {
		return true;
	}
	return false;
}

bool LCU_API::PlayAgain() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret = GET(url("/lol-lobby/v2/play-again"), "");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

Json::Value LCU_API::GetReceivedInvitations() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-lobby/v2/received-invitations"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			return root;
		}
	}
	return NULL;
}

bool LCU_API::AcceptInvitation(const char* invitationId) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/received-invitations/%s/accept";
	char uri_buffer[128];
	sprintf_s(uri_buffer, 128, format, invitationId);
	std::string data = GET(url(uri_buffer), "");
	if (data.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::DeclineInvitation(const char* invitationId) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/received-invitations/%s/decline";
	char uri_buffer[128];
	sprintf_s(uri_buffer, 128, format, invitationId);
	std::string data = GET(url(uri_buffer), "");
	if (data.empty()) {
		return true;
	}
	return false;
}

Json::Value LCU_API::GetAllGridChampions() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-champ-select/v1/all-grid-champions"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			return root;
		}
	}
	return NULL;
}

Json::Value LCU_API::GetBannableChampionIds() {
	if (!IsAPIServerConnected()) {
		return NULL;
	}
	std::string data = GET(url("/lol-champ-select/v1/bannable-champion-ids"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			return root;
		}
	}
	return NULL;
}

int LCU_API::GetCurrentChampion() {
	if (!IsAPIServerConnected()) {
		return 0;
	}
	std::string data = GET(url("/lol-champ-select/v1/current-champion"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		return 0;
	}
	return atoi(data.c_str());
}

bool LCU_API::SetGameSettings() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* content = R"({"Accessibility":{"ColorGamma":"0.0000","ColorContrast":"0.0000","ColorBrightness":"0.0000","ColorLevel":"0.0000"},"Voice":{"InputMode":0,"InputDevice":"Default System Device","InputVolume":"0.5000","ActivationSensitivity":"0.6500"},"General":{"CursorScale":"0.0000","SystemMouseSpeed":10,"WindowMode":2,"Height":768,"Width":1024,"WaitForVerticalSync":0,"SnapCameraOnRespawn":0,"ShowTurretRangeIndicators":0,"ShowGodray":0,"RelativeTeamColors":0,"MinimizeCameraMotion":0,"BindSysKeys":0,"HideEyeCandy":1,"EnableTargetedAttackMove":0,"EnableScreenShake":0,"EnableLightFx":0,"EnableCustomAnnouncer":1,"EnableAudio":0,"AutoAcquireTarget":0,"AlwaysShowExtendedTooltip":0},"HUD":{"DrawHealthBars":1,"ShopScale":"0.0000","PracticeToolScale":"0.0000","MinimapScale":"0.0000","KeyboardScrollSpeed":"0.5000","GlobalScale":"0.0000","DeathRecapScale":"0.0000","ShowSummonerNames":0,"EmoteSize":1,"EmotePopupUIDisplayMode":2,"NumericCooldownFormat":3,"ChatScale":0,"SmartCastOnKeyRelease":0,"ShowSummonerNamesInScoreboard":0,"ObjectTooltips":0,"ShowNeutralCamps":0,"ShowHealthBarShake":0,"ShowAttackRadius":0,"ShowAlliedChat":0,"MinimapMoveSelf":0,"FlashScreenWhenStunned":0,"FlashScreenWhenDamaged":0,"EnableLineMissileVis":0,"AutoDisplayTarget":0},"Performance":{"ShadowQuality":0,"FrameCapType":3,"EnvironmentQuality":0,"EffectsQuality":0,"CharacterQuality":0,"EnableHUDAnimations":0,"EnableGrassSwaying":1,"EnableFXAA":0,"CharacterInking":0},"MapSkinOptions":{"MapSkinOptionDisableWorlds":0,"MapSkinOptionDisableURF":0,"MapSkinOptionDisableStarGuardian":0,"MapSkinOptionDisableSnowdown":0,"MapSkinOptionDisableProject":0,"MapSkinOptionDisablePopstar":0,"MapSkinOptionDisablePoolParty":0,"MapSkinOptionDisableOdyssey":0,"MapSkinOptionDisableMSI":0,"MapSkinOptionDisableLunarRevel":0,"MapSkinOptionDisableArcade":0},"Volume":{"VoiceMute":1,"SfxMute":1,"PingsMute":1,"MusicMute":1,"MasterMute":1,"AnnouncerMute":1,"AmbienceMute":1},"FloatingText":{"Special_Enabled":0,"Score_Enabled":0,"QuestReceived_Enabled":0,"ManaDamage_Enabled":0,"Level_Enabled":0,"Invulnerable_Enabled":0,"Heal_Enabled":0,"Gold_Enabled":0,"Experience_Enabled":0,"EnemyDamage_Enabled":0,"Dodge_Enabled":0,"Damage_Enabled":0},"LossOfControl":{"LossOfControlEnabled":0},"Replay":{"EnableDirectedCamera":0},"Highlights":{"VideoQuality":0,"VideoFrameRate":60,"ScaleVideo":720,"AudioQuality":1}})";
	std::string data = PATCH(url("/lol-game-settings/v1/game-settings"), content);
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return true;
		}
	}
	return false;
}

bool LCU_API::Reconnect() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string data = POST(url("/lol-gameflow/v1/reconnect"), "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return true;
		}
	}
	return false;
}

bool LCU_API::DeclineSearch() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret = POST(url("/lol-matchmaking/v1/ready-check/decline"), "");
	if (ret.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::AcceptSearch() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret = POST(url("/lol-matchmaking/v1/ready-check/accept"), "");
	if (ret.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::KillUx() {
	std::string ret = POST(url("/riotclient/kill-ux"), "");
	if (ret.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::ShowUx() {
	std::string ret = POST(url("/riotclient/ux-show"), "");
	if (ret.empty()) {
		return true;
	}
	return false;
}