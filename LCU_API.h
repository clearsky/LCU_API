#pragma once
#include <cpprest/ws_client.h> 
#include "HTTP.h"
#include "Auth.h"
#include "Definitions.h"
#include <unordered_map>
#include "json/json.h"

#define USE_LOG  // ������ӡ����ֵ��������Ϣ
#ifdef USE_LOG
//#define JSON_FORMAT // ��ʽ�����json
#define USE_GBK // ������gbk��������������
//#define LOG_ALL_EVENT
#endif // USE_LOG

typedef void (CALLBACK* EVENT_CALLBACK)(Json::Value &data);

enum EventType {
	CREATE,
	UPDATE,
	DEL,
	ALL
};


namespace LCUAPI {
class LCU_API
	{
	public:
		LCU_API();
		~LCU_API();
	private:
		std::string buildRoomApi;
		std::string startQueueApi;
	public:
		std::unordered_map<std::string, EVENT_CALLBACK[3]> filter;
		Auth auth;
		web::websockets::client::websocket_callback_client *ws_client;
		HTTP http_client;
		bool connected;
		std::string host;
		std::string prot[2];
		std::function<std::string&(const std::string&, const std::string&)> GET;
		std::function<std::string&(const std::string&, const std::string&)> POST;
		std::function<std::string&(const std::string&, const std::string&)> PUT;
		std::function<std::string&(const std::string&, const std::string&)> DELETe;
	private:
		bool Connect();
	public:
		void HandleEventMessage(web::websockets::client::websocket_incoming_message& msg);
		bool AddEventFilter(std::string uri, EventType type, EVENT_CALLBACK call_back);
		bool RemoveEventFilter(std::string uri, EventType type);
		bool IsAPIServerConnected(); // api�Ƿ�����
		template<typename T>
		bool ResultCheck(const std::string& data,const std::string& attr, T aim); // ����ֵ���
		std::string url(const std::string& api);
		bool BuildRoom(QueueID type); // ��������
		bool BuildTFTNormalRoom(); // �����ƶ�ƥ��
		bool BuildTFTRankRoom(); // �����ƶ���λ
		bool StartQueue(); // ��ʼƥ��
		std::string &Request(const std::string &method, const std::string &url, const std::string &requestData = "", const std::string &header = "",
			const std::string& cookies = "", const std::string &returnCookies = "", int port = -1);
};
}

