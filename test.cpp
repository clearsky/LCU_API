#include "LCU_API.h"

int main() {
	/*LCUAPI::LCU_API api(FILTER);
	api.AddEventFilter("/lol-summoner/v1/current-summoner", UPDATE, (EVENT_CALLBACK)[](Json::Value &data) {
		std::cout << "The summoner " << data[2]["data"]["displayName"] << " was updated." << std::endl;
	});*/
	LCUAPI::LCU_API api(BIND);
	api.BindEvent("OnJsonApiEvent", [](Json::Value& data) {
		std::cout << "callback called" << std::endl;
	});
	api.BuildTFTRankRoom();
	system("pause");
}