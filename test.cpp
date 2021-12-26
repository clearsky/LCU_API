#include "LCU_API.h"

int main() {
	LCUAPI::LCU_API api;
	api.AddEventFilter("/lol-summoner/v1/current-summoner", UPDATE, (EVENT_CALLBACK)[](Json::Value &data) {
		std::cout << "The summoner " << data[2]["data"]["displayName"] << " was updated." << std::endl;
	});
	api.BuildTFTRankRoom();
	system("pause");
}