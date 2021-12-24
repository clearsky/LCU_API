#pragma once
#include "Definitions.h"
#include "Auth.h"
#include "HTTP.h"
#include <functional>
#include "Utils.h"

#define USE_LOG  // ������ӡ����ֵ��������Ϣ
#ifdef USE_LOG
#define JSON_FORMAT // ��ʽ�����json
#endif // USE_LOG


//#define USE_DLL // DLL��ʽ����
#ifdef USE_DLL 
#define LCU_EXPORTS // ͷ�ļ��ǵ�����,����ʹ��DLLʱ��
#ifdef __cplusplus
extern "C" {
#endif
#ifdef LCU_EXPORTS
#define LCU_API __declspec(dllexport) 
#else
#define LCU_API __declspec(dllimport) 
#endif // LCU_EXPORTS
class LCU_API API;
#ifdef __cplusplus
}
#endif
#endif // USE_DLL 


class  API
{
private:
	Auth auth;
	HTTP http;
	bool connected;
	std::string host;
	std::string buildRoomApi;
	std::string startQueueApi;
	std::string result;
	std::function<std::string(std::string, std::string)> GET;
	std::function<std::string(std::string, std::string)> POST;
	std::function<std::string(std::string, std::string)> PUT;
	std::function<std::string(std::string, std::string)> DELETe;
public:
	API();
	bool IsAPIServerConnected(); // api�Ƿ�����
	template<typename T>
	bool ResultCheck(std::string attr, T aim); // ����ֵ���
#ifdef USE_LOG
	std::string RequestWithLog(std::string method, std::string url, std::string requestData = "", std::string header = "",
		std::string cookies = "", std::string returnCookies = "", int port = -1);
#endif // USE_LOG
	bool BuildRoom(QueueID type); // ��������
	bool BuildTFTNormalRoom(); // �����ƶ�ƥ��
	bool BuildTFTRankRoom(); // �����ƶ���λ
	bool StartQueue(); // ��ʼƥ��
};

