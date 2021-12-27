#include "LCU_API.h"
#include <thread>
#include <fstream>

using namespace LCUAPI;
int main() {
	/*LCUAPI::LCU_API api(FILTER);
	api.AddEventFilter("/lol-summoner/v1/current-summoner", UPDATE, (EVENT_CALLBACK)[](Json::Value &data) {
		std::cout << "The summoner " << data[2]["data"]["displayName"] << " was updated." << std::endl;
	});*/
	LCU_API api(BIND);
	api.Connect();
	/*api.ExitRoom();*/
	api.SetMetaData(PositionPref::FILL, PositionPref::FILL);
	
	/*api.BindEvent("OnJsonApiEvent", [](Json::Value& data) {
		std::cout << "==========all event============" << std::endl;
		std::cout << data.toStyledString() << std::endl;
		std::ofstream out("ret.txt", std::ios::app);
		out << data.toStyledString();
	});*/
	//api.OnCreateRoom([](Json::Value& data) {
	//	::MessageBoxA(0, "room created", 0, 0);
	//});
	//api.OnCloseRoom([](Json::Value& data) {
	//	::MessageBoxA(0, "close created", 0, 0);
	//});
	//while (1) {
	//	if (!api.connected) {
	//		api.Connect();
	//	}
	//	Sleep(1000);
	//}
	system("pause");
	
}