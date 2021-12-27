#pragma once
#include <cpprest/ws_client.h> 
#include "HTTP.h"
#include "Auth.h"
#include "Definitions.h"
#include <unordered_map>
#include "json/json.h"
#include <vector>

#define USE_LOG  // print log
#ifdef USE_LOG
//#define JSON_FORMAT // format json result
#define USE_GBK // gbk log
//#define LOG_ALL_EVENT
#endif // USE_LOG



namespace LCUAPI {

	typedef std::function<void (Json::Value& data)>  EVENT_CALLBACK;

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
	public:
		std::unordered_map<std::string, EVENT_CALLBACK[3]> filter;
		std::unordered_map<std::string, EVENT_CALLBACK> bind;
		std::unordered_map<std::string, EVENT_CALLBACK[3]> binded_event;
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

	public:
		bool Connect();
		void HandleEventMessage(web::websockets::client::websocket_incoming_message& msg);
		bool BindEvent(const std::string & method, EVENT_CALLBACK call_back, bool force=false);
		bool UnBindEvent(const std::string & method);
		bool AddEventFilter(const std::string &uri, EventType type, EVENT_CALLBACK call_back);
		bool RemoveEventFilter(const std::string &uri, EventType type);
		bool IsAPIServerConnected(); // api is connected
		template<typename T>
		bool ResultCheck(const std::string& data,const std::string& attr, T aim); // check result
		std::string url(const std::string& api);
		std::string Request(const std::string &method, const std::string &url, const std::string &requestData = "", const std::string &header = "",
			const std::string& cookies = "", const std::string &returnCookies = "", int port = -1);
		bool BindEventAuto(const std::string& event_name, const std::string& uri);
		// EVENT
		bool OnCreateRoom(EVENT_CALLBACK call_back);
		bool OnCloseRoom(EVENT_CALLBACK call_back);
		bool OnUpdateRoom(EVENT_CALLBACK call_back);
		// API
		// lobby
		// /lol-lobby/v2/lobby
		bool BuildRoom(QueueID type); 
		bool BuildTFTNormalRoom(); 
		bool BuildTFTRankRoom(); 
		bool ExitRoom();
		Json::Value GetRoom();

		// /lol-lobby/v2/lobby/partyType
		bool OpenTeam();
		bool CloseTeam();

		// /lol-lobby/v1/lobby/availability
		// 判断是否在房间
		bool LobbyAvailability();

		// /lol-lobby/v1/lobby/invitations
		// 房间右下角的邀请信息
		Json::Value GetInvitations();

		// /lol-lobby/v1/parties/gamemode
		// 获取当前房间的游戏模式信息
		Json::Value GetGameMode();

		// /lol-lobby/v1/parties/metadata
		// 排位的时候选位置
		bool SetMetaData(PositionPref first, PositionPref second);


		bool StartQueue(); 
};
}

