#include "lcuapi/LCU_API.h"
#include "utils/utils.h"
int main() {
	LCUAPI::LCU_API api;
	while (1) {
		if (utils->GetProcessidFromName(L"LeagueClient.exe")) {
			// 连接LCU
			if (!api.IsAPIServerConnected()) {
				//sLog.emit(L"未连接到LCU");
				for (int i = 0; i < 60; ) {
					printf("not connected\n");
					//sLog.emit(L"正在连接LCU...");
					api.Connect();
					//sLog.emit(L"连接完成");
					if (api.IsAPIServerConnected()) {
						printf("connected\n");
						//init_after_connected();
						break;
					}
					//if (i == 59) {
						//sLog.emit(L"多次连接失败,重启游戏");
						//restart_game();
					//}
					//sleep(1000);
					Sleep(1000);
					/*if (!utils->GetProcessidFromName(L"LeagueClient.exe")) {
						goto go;
					}*/
				}
			}
			// 状态路由
			LCUAPI::GameFlowPhase status = api.GetGameflowPhase();
			//if (open_group && !is_captain) { // 队员自动接受邀请
			//	Json::Value invited_data = api->GetReceivedInvitations();
			//	if (!invited_data.empty() && !invited_data[0].isNull()) {
			//		for (auto item : invited_data) {
			//			if (!item.isMember("fromSummonerId")) {
			//				break;
			//			}
			//			if (item["fromSummonerId"].asInt64() == summonerids[1]) {
			//				api->AcceptInvitation(item["invitationId"].asString().c_str());
			//			}
			//			else {
			//				api->DeclineInvitation(item["invitationId"].asString().c_str());
			//			}
			//		}

			//	}
			//}
			switch (status)
			{
			case LCUAPI::GameFlowPhase::EndOfGame:  // 游戏结束
			{
				// 代币估算
				//Json::Value data = api.GetEndOfGameData();
				//if (!data.empty()) {
				//	if (data.isMember("statsBlock") && data["statsBlock"].isMember("players") &&
				//		data["statsBlock"].isMember("players") && data["statsBlock"]["players"].isArray()) {
				//		for (auto item : data["statsBlock"]["players"]) {
				//			if (item["playerId"].asInt64() == summonerids[0]) {
				//				add_coin = 0;
				//				switch ((5 - ((item["ffaStanding"].asInt() + 1) / 2)))
				//				{
				//				case 1:
				//					add_coin = 1;
				//					break;
				//				case 2:
				//					add_coin = 2;
				//					break;
				//				case 3:
				//					add_coin = 4;
				//					break;
				//				case 4:
				//					add_coin = 6;
				//					break;
				//				default:
				//					break;
				//				};
				//				//sPush.emit(push_qq, qq, add_coin);
				//				break;
				//			}
				//		}
				//	}
				//}
				//else {
				//	continue;
				//}
				break;
			}
			case LCUAPI::GameFlowPhase::None: // 在大厅
				break;
			case LCUAPI::GameFlowPhase::InProgress: // 游戏中
			{
				
			}
			break;
			case LCUAPI::GameFlowPhase::Lobby:  // 房间中
			{
		
			}
			break;
			case LCUAPI::GameFlowPhase::Reconnect: {
				api.Reconnect();
			}
			break;
			case LCUAPI::GameFlowPhase::ReadyCheck: { // 自动接受
				api.AcceptSearch();
			}
			break;
			case LCUAPI::GameFlowPhase::Matchmaking: { // 队列中
				
			}
		    break;
			default:
				break;
			}
		}
		Sleep(500);
	}
	return 0;
}