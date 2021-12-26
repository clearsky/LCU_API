#pragma once
#include <cpprest/ws_client.h> 
#include "HTTP.h"
#include "Auth.h"
#include "Definitions.h"
#include <unordered_map>
#include "json/json.h"
#include <vector>

#define USE_LOG  // 请求后打印返回值和请求信息
#ifdef USE_LOG
//#define JSON_FORMAT // 格式化输出json
#define USE_GBK // 避免在gbk环境中中文乱码
//#define LOG_ALL_EVENT
#endif // USE_LOG



namespace LCUAPI {

	typedef std::function<void(Json::Value& data)> CALLBACK EVENT_CALLBACK;

	enum EventType {
		CREATE,
		UPDATE,
		DEL,
		ALL
	};
	enum EventHandleType
	{
		FILTER,
		BIND
	};

class LCU_API
	{
	public:
		LCU_API(EventHandleType event_handle_type);
		~LCU_API();
	private:
		EventHandleType event_handle_type;
		std::string buildRoomApi;
		std::string startQueueApi;
	public:
		std::unordered_map<std::string, EVENT_CALLBACK[3]> filter;
		std::unordered_map<std::string, EVENT_CALLBACK> bind;
		//std::unordered_map<bool(LCU_API::*)(EVENT_CALLBACK, EVENT_CALLBACK, EVENT_CALLBACK), EVENT_CALLBACK[3]> binded_event;
		Auth auth;
		web::websockets::client::websocket_callback_client *ws_client;
		HTTP http_client;
		bool connected;
		std::string host;
		std::string prot[2];
		std::function<std::string(const std::string&, const std::string&)> GET;
		std::function<std::string(const std::string&, const std::string&)> POST;
		std::function<std::string(const std::string&, const std::string&)> PUT;
		std::function<std::string(const std::string&, const std::string&)> DELETe;
	private:
		bool Connect();
	public:
		void HandleEventMessage(web::websockets::client::websocket_incoming_message& msg);
		bool BindEvent(const std::string & method, EVENT_CALLBACK call_back);
		bool UnBindEvent(const std::string & method);
		bool AddEventFilter(const std::string &uri, EventType type, EVENT_CALLBACK call_back);
		bool RemoveEventFilter(const std::string &uri, EventType type);
		bool IsAPIServerConnected(); // api是否连接
		template<typename T>
		bool ResultCheck(const std::string& data,const std::string& attr, T aim); // 返回值检查
		std::string url(const std::string& api);
		std::string Request(const std::string &method, const std::string &url, const std::string &requestData = "", const std::string &header = "",
			const std::string& cookies = "", const std::string &returnCookies = "", int port = -1);

		// EVENT
		bool OnJsonApiEvent_lol_lobby_v2_lobby(EVENT_CALLBACK create_call = nullptr, EVENT_CALLBACK update_call = nullptr, EVENT_CALLBACK delete_call = nullptr);
		// API
		bool BuildRoom(QueueID type); // 创建房间
		bool BuildTFTNormalRoom(); // 创建云顶匹配
		bool BuildTFTRankRoom(); // 创建云顶排位
		bool StartQueue(); // 开始匹配
};
}

