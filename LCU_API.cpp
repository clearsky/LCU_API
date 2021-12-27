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
	if (event_handle_type != BIND) {
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
	if (event_handle_type != FILTER) {
		std::cout << "event_handle_type is not filter" << std::endl;
		return false;
	}
	switch (type)
	{
	case CREATE:
		filter[uri][0] = call_back;
		break;
	case UPDATE:
		filter[uri][1] = call_back;
		break;
	case DEL:
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
	case CREATE:
		filter.at(uri)[0] = nullptr;
		break;
	case UPDATE:
		filter.at(uri)[1] = nullptr;
		break;
	case DEL:
		filter.at(uri)[2] = nullptr;
		break;
	case ALL:
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
// Á¬½ÓLCU
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

		// check lol client is ready
		std::string load_ok_str = GET(url("/memory/v1/fe-processes-ready"), "");
		if (load_ok_str != "true") {
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

		if (event_handle_type == FILTER) {
			websocket_outgoing_message msg;
			msg.set_utf8_message("[5, \"OnJsonApiEvent\"]");
			auto sub_ret = ws_client->send(msg).then([]() {
				std::cout << "subscribed all events" << std::endl;
				});
			sub_ret.wait();
		}
		else if (event_handle_type == BIND) { // rebind events
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
			if (event_handle_type == FILTER) {
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
			else if (event_handle_type == BIND) {
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
bool LCU_API::StartQueue() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	if (POST(url("/matchmaking/search"), "").empty()) {
		return true;
	}
	else {
		return false;
	}
}

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
	DELETe(url("/lol-lobby/v2/lobby"), "");
	return true;
}

Json::Value LCU_API::GetRoom() {
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
	std::string ret = PUT(url("/lol-lobby/v2/lobby/partyType"), R"("open")");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

bool LCU_API::CloseTeam() {
	std::string ret = PUT(url("/lol-lobby/v2/lobby/partyType"), R"("closed")");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

bool LCU_API::LobbyAvailability() {
	std::string ret = GET(url("/lol-lobby/v1/lobby/availability"), "");
	if (ret == R"("Available")") {
		return true;
	}
	else {
		return false;
	}
}

Json::Value LCU_API::GetInvitations() {
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
	const char* types[] = { "TOP", "JUNGLE", "MIDDLE", "BOTTOM", "UTILITY", "FILL", "UNSELECTED" };
    char const* format = R"({"championSelection" : null,"positionPref" :["%s","%s"] ,"properties" : null,"skinSelection" : null})";
	char data[128];
	sprintf_s(data, 128, format, types[static_cast<int>(first)], types[static_cast<int>(second)]);
	std::string ret = PUT(url("/lol-lobby/v1/parties/metadata"), data);
	return true;
}