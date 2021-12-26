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

LCU_API::LCU_API() {
	connected = false;
	host = "127.0.0.1";
	prot[0] = "https://";
	prot[1] = "wss://";
	buildRoomApi = "/lol-lobby/v2/lobby";
	startQueueApi = "/matchmaking/search";
}

bool LCU_API::AddEventFilter(std::string uri, EventType type, EVENT_CALLBACK call_back) {
	if (filter.find(uri) != filter.end()) {
		switch (type)
		{
		case CREATE:
			filter.at(uri)[0] = call_back;
			break;
		case UPDATE:
			filter.at(uri)[1] = call_back;
			break;
		case DEL:
			filter.at(uri)[2] = call_back;
			break;
		default:
			return false;
		}
	}
	else {
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
	}
	return true;
}

bool LCU_API::RemoveEventFilter(std::string uri, EventType type) {
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
	return true;
}
// 连接LCU
bool LCU_API::Connect() {
	if (!auth.GetLeagueClientInfo()) {
		return false;
	}
	if (ws_client) {
		delete ws_client;
	}
	websocket_client_config ws_config;
	ws_config.set_validate_certificates(false);
	std::string auth_str = "Basic ";
	auth_str += auth.leagueToken;
	ws_config.headers()[L"Authorization"] = CA2W(auth_str.c_str());

	ws_client = new websocket_callback_client(ws_config);

	std::string ws_str = prot[1] + host + ":" + std::to_string(auth.leaguePort);
	wchar_t* ws_wstr = CA2W(ws_str.c_str());
	try {
		auto con_ret = ws_client->connect(ws_wstr).then([]() {
			std::cout << "wss connected" << std::endl;
		});
		con_ret.wait();

		websocket_outgoing_message msg;
		msg.set_utf8_message("[5, \"OnJsonApiEvent\"]");
		auto sub_ret = ws_client->send(msg).then([]() {
			std::cout << "subscribed all events" << std::endl;
		});
		sub_ret.wait();

		ws_client->set_message_handler([this](websocket_incoming_message msg){
			this->HandleEventMessage(msg);
		});
		ws_client->set_close_handler([this](websocket_close_status close_status,const utility::string_t& reason,const std::error_code& error) {
			this->connected = false;
			std::cout << "wss connect closed" << std::endl;
		});
	}
	catch (websocket_exception const& ex) {
		std::cout << ex.what() << std::endl;
		return false;
	}
	return true;
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

// 检测是否连接LCU
bool LCU_API::IsAPIServerConnected() {
	if (!connected) {
		connected = Connect();
		if (connected) {
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
		}
	}
	if (!connected) {
		std::cout << "lcu not connected" << std::endl;
	}
	return connected;
}

// URL拼接
std::string LCU_API::url(const std::string &api) {
	return prot[0] + host + api;
}

// 开始匹配
bool LCU_API::StartQueue() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	if (POST(url(startQueueApi), "").empty()) {
		return true;
	}
	else {
		return false;
	}
}

// 创建房间
bool LCU_API::BuildRoom(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string body = R"({"queueId":)" + std::to_string(static_cast<int>(type)) + "}";
	return ResultCheck(POST(url(buildRoomApi), body), "canStartActivity", true);
}

// 创建云顶匹配房间
bool LCU_API::BuildTFTNormalRoom() {
	return BuildRoom(TFTNormal);
}

// 创建云顶排位房间
bool LCU_API::BuildTFTRankRoom() {
	return BuildRoom(TFTRanked);
}

std::string &LCU_API::Request(const std::string& method, const std::string& url, const std::string& requestData, const std::string& header,
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
	// 检测结果
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