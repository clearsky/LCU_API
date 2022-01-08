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
	enum class SearchState
	{
		Invalid,
		Searching,
		Found
	};
	enum class EventType {
		CREATE,
		UPDATE,
		DEL,
		ALL
	};
	enum class PartyType {
		OPEN,CLOSED
	};
	enum class EventHandleType
	{
		FILTER,
		BIND
	};
	enum class PositionPref {
		TOP, JUNGLE, MIDDLE, BOTTOM, UTILITY, FILL, UNSELECTED
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
		std::function<std::string(const std::string&, const std::string&)> PATCH;
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
		// EVENT
		bool BindEventAuto(const std::string& event_name, const std::string& uri);
		bool OnCreateRoom(EVENT_CALLBACK call_back);
		bool OnCloseRoom(EVENT_CALLBACK call_back);
		bool OnUpdateRoom(EVENT_CALLBACK call_back);
		bool OnCreateSearch(EVENT_CALLBACK call_back);
		bool OnUpdateSearch(EVENT_CALLBACK call_back);
		bool OnDeleteSearch(EVENT_CALLBACK call_back);
		// API
		// ======================lobby=========================
		// /lol-lobby/v2/lobby
		bool BuildRoom(QueueID type); 
		bool BuildTFTNormalRoom(); 
		bool BuildTFTRankRoom(); 
		bool ExitRoom();
		Json::Value GetRoom();
		// /lol-lobby/v2/lobby/partyType
		bool OpenTeam();
		bool CloseTeam();
		// /lol-lobby/v1/lobby/availability 判断是否在房间
		bool LobbyAvailability();
		// /lol-lobby/v1/lobby/invitations 自定义房间右下角的邀请信息
		Json::Value GetInvitations();
		// /lol-lobby/v1/parties/gamemode 获取当前房间的游戏模式信息
		Json::Value GetGameMode();
		// /lol-lobby/v1/parties/metadata 排位的时候选位置
		bool SetMetaData(PositionPref first, PositionPref second);
		// /lol-lobby/v1/parties/player 获取房间玩家的信息,还包含其他信息,在房间外也可以获取
		Json::Value GetPartiedPlayer();
		// /lol-lobby/v1/parties/queue 切换房间模式
		bool SetQueue(QueueID type);
		// /lol-lobby/v1/parties/{partyId}/members/{puuid}/role 转移房主
		bool GiveLeaderRole(const char* party_id, const char* puuid);
		// lol-lobby/v2/comms/members 获取房间内可聊天的playeri信息,比GetPartiedPlayer更轻量
		Json::Value GetCommsMembers();
		// /lol-lobby/v2/comms/token 获取comm toke
		std::string GetCommsToken();
		// /lol-lobby/v2/eligibility/party 查询当前房间是否有资格转换到其他类型的房间
		bool CheckPartyEligibility(QueueID type);
		// /lol-lobby/v2/eligibility/self 自己能否创建某种类型的房间
		bool CheckSelfEligibility(QueueID type);
		// /lol-lobby/v2/lobby/invitations 房间右下角邀请信息
		Json::Value GetLobbyInvitations();
		// /lol-lobby/v2/lobby/invitations 邀请
		bool InviteBySummonerIdS(std::vector<long long> ids);
		// /lol-lobby/v2/lobby/matchmaking/search POST开始匹配
		bool StartQueue();
		// /lol-lobby/v2/lobby/matchmaking/search DELETE终止匹配
		bool StopQueue();
		// /lol-lobby/v2/lobby/matchmaking/search-state 获取匹配状态
		SearchState GetSearchState();
		// /lol-lobby/v2/lobby/members 获取房间内玩家信息
		Json::Value GetLobbyMembers();
		// /lol-lobby/v2/lobby/members/localMember/position-preferences 设置排位预选位
		bool SetPositionPreferences(PositionPref first, PositionPref second);
		// /lol-lobby/v2/lobby/members/{summoner ID}/grant-invite 授予邀请权限
		bool GrantInviteBySummonerId(long long id);
		// /lol-lobby/v2/lobby/members/{summoner ID}/kick 踢出房间
		bool KickSummonerBySummonerId(long long id);
		// /lol-lobby/v2/lobby/members/{summoner ID}/promote 转移房主权限
		bool PromoteSummonerBySummonerId(long long id);
		// /lol-lobby/v2/lobby/members/{summoner ID}/revoke-invite 收回邀请权限
		bool RevokeInviteBySummonerId(long long id);
		// /lol-lobby/v2/lobby/partyType 设置小队状态
		bool SetPartyType(PartyType type);
		// /lol-lobby/v2/party-active 是否在房间中
		bool IsPartyActive();
		// /lol-lobby/v2/play-again 再玩一次
		bool PlayAgain();
		// /lol-lobby/v2/received-invitations 被邀请信息
		Json::Value GetReceivedInvitations();
		// /lol-lobby/v2/received-invitations/{invitationId}/accept // 接受邀请
		bool AcceptInvitation(const char* invitationId);
		// /lol-lobby/v2/received-invitations/{invitationId}/decline // 拒绝邀请
		bool DeclineInvitation(const char* invitationId);
		// ===============champ-select=============
		// /lol-champ-select/v1/all-grid-champions 获取所有英雄信息
		Json::Value GetAllGridChampions();
		// /lol-champ-select/v1/bannable-champion-ids 可以ban的英雄列表
		Json::Value GetBannableChampionIds();
		// /lol-champ-select/v1/current-champion 当前选择的英雄ID
		int GetCurrentChampion();
		// ===============game-setting=============
		// /lol-game-settings/v1/game-settings
		bool SetGameSettings();
		// /lol-gameflow/v1/reconnect 游戏重连
		bool Reconnect();
		// ===============matchmaking=============
		// /lol-matchmaking/v1/ready-check/decline 拒绝队列
		bool DeclineSearch();
		// /lol-matchmaking/v1/ready-check/accept 接受队列
		bool AcceptSearch();
		// ===============default=============
		// /riotclient/kill-ux 隐藏界面
		bool KillUx();
		// /riotclient/ux-show 显示界面
		bool ShowUx();
};
}

