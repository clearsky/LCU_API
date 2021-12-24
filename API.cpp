#include "API.h"
#include "json/json.h"

#define HOST 127.0.0.1

int testf(int a, int b=1) {
	return 0;
}
API::API() {
	connected = false;
	host = "127.0.0.1";
	buildRoomApi = "https://127.0.0.1/lol-lobby/v2/lobby";
	startQueueApi = "https://127.0.0.1/lol-lobby/v2/lobby/matchmaking/search";

	
}

#ifdef  USE_LOG
	std::string API::RequestWithLog(std::string method, std::string url, std::string requestData, std::string header ,
		std::string cookies, std::string returnCookies, int port) {
		std::string result = http.Request(method, url, requestData, header, cookies, returnCookies, port);
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
		return result;
	}
#endif //  USE_LOG


bool API::IsAPIServerConnected() {
	if (!connected) {
		connected = auth.GetLeagueClientInfo();
		if (connected) {
			
#ifdef USE_LOG
			auto request_call = &API::RequestWithLog;
			auto obj = this;
#elif
			auto request_call = &HTTP::Request;
			auto obj = http;
#endif // USE_LOG
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
	return connected;
}

bool API::StartQueue() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	result = POST(startQueueApi, "");
	if (result.empty()) {
		return true;
	}
	else {
		return false;
	}
}

bool API::BuildRoom(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string body = R"({"queueId":)" + std::to_string(1090) + "}";
	result = POST(buildRoomApi, body);
	return ResultCheck("canStartActivity", true);
}

template<typename T>
bool API::ResultCheck(std::string attr, T aim) {
	bool ret = false;
	// ¼ì²â½á¹û
	if (!result.empty())
	{
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		JSONCPP_STRING err;
		Json::Value root;
		if (!reader->parse(result.c_str(), result.c_str() + static_cast<int>(result.length()-1), &root, &err)) {
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

bool API::BuildTFTNormalRoom() {
	return BuildRoom(TFTNormal);
}

bool API::BuildTFTRankRoom() {
	return BuildRoom(TFTRanked);
}