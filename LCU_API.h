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
		// ============lobby===============
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
		// �ж��Ƿ��ڷ���
		bool LobbyAvailability();

		// /lol-lobby/v1/lobby/invitations
		// �Զ��巿�����½ǵ�������Ϣ
		Json::Value GetInvitations();

		// /lol-lobby/v1/parties/gamemode
		// ��ȡ��ǰ�������Ϸģʽ��Ϣ
		Json::Value GetGameMode();

		// /lol-lobby/v1/parties/metadata
		// ��λ��ʱ��ѡλ��
		bool SetMetaData(PositionPref first, PositionPref second);
		
		// /lol-lobby/v1/parties/player
		// ��ȡ������ҵ���Ϣ,������������Ϣ,�ڷ�����Ҳ���Ի�ȡ
		Json::Value GetPartiedPlayer();

		// /lol-lobby/v1/parties/queue
		// �л�����ģʽ
		bool SetQueue(QueueID type);

		// /lol-lobby/v1/parties/{partyId}/members/{puuid}/role
		// ת�Ʒ���
		bool GiveLeaderRole(const char* party_id, const char* puuid);

		// lol-lobby/v2/comms/members 
		//  ��ȡ�����ڿ������playeri��Ϣ,��GetPartiedPlayer������
		Json::Value GetCommsMembers();

		// /lol-lobby/v2/comms/token ��ȡcomm toke
		std::string GetCommsToken();

		// /lol-lobby/v2/eligibility/party ��ѯ��ǰ�����Ƿ����ʸ�ת�����������͵ķ���
		bool CheckPartyEligibility(QueueID type);

		// /lol-lobby/v2/eligibility/self �Լ��ܷ񴴽�ĳ�����͵ķ���
		bool CheckSelfEligibility(QueueID type);

		bool StartQueue();

};
}

