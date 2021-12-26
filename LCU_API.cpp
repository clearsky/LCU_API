#include "LCU_API.h"
#include "atlbase.h"
#include "atlstr.h"
#include <string>
#include "json/json.h"


using namespace LCUAPI;
using namespace std;
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

bool LCU_API::AddEventFilter(std::string uri, EventType type) {
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
	string auth_str = "Basic ";
	auth_str += auth.leagueToken;
	ws_config.headers()[L"Authorization"] = CA2W(auth_str.c_str());

	ws_client = new websocket_callback_client(ws_config);

	string ws_str = prot[1] + host + ":" + to_string(auth.leaguePort);
	wchar_t* ws_wstr = CA2W(ws_str.c_str());
	try {
		auto con_ret = ws_client->connect(ws_wstr).then([]() {
			cout << "wss connected" << endl;
		});
		con_ret.wait();

		websocket_outgoing_message msg;
		msg.set_utf8_message("[5, \"OnJsonApiEvent\"]");
		ws_client->send(msg).then([]() {
			cout << "subscribed all events" << endl;
		});

		ws_client->set_message_handler([this](websocket_incoming_message msg){
			this->HandleEventMessage(msg);
		});
		ws_client->set_close_handler([this](websocket_close_status close_status,const utility::string_t& reason,const std::error_code& error) {
			this->connected = false;
			cout << "wss connect close" << endl;
		});
	}
	catch (websocket_exception const& ex) {
		std::cout << ex.what() << std::endl;
		return false;
	}
	return true;
}

void LCU_API::HandleEventMessage(websocket_incoming_message& msg) {
	cout << msg.extract_string().get() << endl;
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
		cout << "lcu not connected" << endl;
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
	result = POST(url(startQueueApi), "");
	if (result.empty()) {
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
	std::string body = R"({"queueId":)" + std::to_string(1090) + "}";
	result = POST(url(buildRoomApi), body);
	return ResultCheck("canStartActivity", true);
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

		std::cout << "Request Data:" << std::endl << content << std::endl;
	}
	if (!result.empty()) {
		std::string content;
#ifdef JSON_FORMAT
		content = utils->formatJson(result);
#else
		content = result;
#endif // JSON_FORMAT
		std::cout << "Result:" << std::endl << content << std::endl;
	}
#endif //  USE_LOG
	return result;
}


template<typename T>
bool LCU_API::ResultCheck(const std::string& attr, T aim) {
	bool ret = false;
	// 检测结果
	if (!result.empty())
	{
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		JSONCPP_STRING err;
		Json::Value root;
		if (!reader->parse(result.c_str(), result.c_str() + static_cast<int>(result.length() - 1), &root, &err)) {
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
		result = "";
	}
	return ret;
}