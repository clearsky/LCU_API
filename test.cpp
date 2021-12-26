#include "LCU_API.h"

using namespace LCUAPI;
int main() {
	/*LCUAPI::LCU_API api(FILTER);
	api.AddEventFilter("/lol-summoner/v1/current-summoner", UPDATE, (EVENT_CALLBACK)[](Json::Value &data) {
		std::cout << "The summoner " << data[2]["data"]["displayName"] << " was updated." << std::endl;
	});*/
	LCU_API api(BIND);
	api.OnJsonApiEvent_lol_lobby_v2_lobby(
		[](Json::Value data) {::MessageBoxA(0, "create room", 0, 0);}, 
		nullptr, 
		[](Json::Value data) {::MessageBoxA(0, "delete room", 0, 0);});
	api.BuildTFTRankRoom();
	system("pause");
}