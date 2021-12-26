#pragma once
#include <cpprest/ws_client.h> 
#include "HTTP.h"
#include "Auth.h"
#include "Definitions.h"

#define USE_LOG  // ������ӡ����ֵ��������Ϣ
#ifdef USE_LOG
//#define JSON_FORMAT // ��ʽ�����json
#endif // USE_LOG

enum EventType {
	CREATE,
	UPDATE,
	DEL
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
		Auth auth;
		web::websockets::client::websocket_callback_client *ws_client;
		HTTP http_client;
		bool connected;
		std::string host;
		std::string prot[2];
		std::string result;
		std::function<std::string&(const std::string&, const std::string&)> GET;
		std::function<std::string&(const std::string&, const std::string&)> POST;
		std::function<std::string&(const std::string&, const std::string&)> PUT;
		std::function<std::string&(const std::string&, const std::string&)> DELETe;
	private:
		bool Connect();
	public:
		void HandleEventMessage(web::websockets::client::websocket_incoming_message& msg);
		bool AddEventFilter(std::string uri, EventType type);
		bool IsAPIServerConnected(); // api�Ƿ�����
		template<typename T>
		bool ResultCheck(const std::string& attr, T aim); // ����ֵ���
		std::string url(const std::string& api);
		bool BuildRoom(QueueID type); // ��������
		bool BuildTFTNormalRoom(); // �����ƶ�ƥ��
		bool BuildTFTRankRoom(); // �����ƶ���λ
		bool StartQueue(); // ��ʼƥ��
		std::string &Request(const std::string &method, const std::string &url, const std::string &requestData = "", const std::string &header = "",
			const std::string& cookies = "", const std::string &returnCookies = "", int port = -1);
};
}

