#pragma once
#include "Definitions.h"
#include "Auth.h"
#include "HTTP.h"
#include <functional>
#include "Utils.h"

#define USE_LOG  // 请求后打印返回值和请求信息
#ifdef USE_LOG
#define JSON_FORMAT // 格式化输出json
#endif // USE_LOG


//#define USE_DLL // DLL方式编译
#ifdef USE_DLL 
#define LCU_EXPORTS // 头文件是导出用,还是使用DLL时用
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
	bool IsAPIServerConnected(); // api是否连接
	template<typename T>
	bool ResultCheck(std::string attr, T aim); // 返回值检查
#ifdef USE_LOG
	std::string RequestWithLog(std::string method, std::string url, std::string requestData = "", std::string header = "",
		std::string cookies = "", std::string returnCookies = "", int port = -1);
#endif // USE_LOG
	bool BuildRoom(QueueID type); // 创建房间
	bool BuildTFTNormalRoom(); // 创建云顶匹配
	bool BuildTFTRankRoom(); // 创建云顶排位
	bool StartQueue(); // 开始匹配
};

