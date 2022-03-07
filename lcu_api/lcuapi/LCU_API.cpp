#include "LCU_API.h"
#include "atlbase.h"
#include "atlstr.h"
#include "utils/utils.h"
#include <string>
#include "json/json.h"
#include <functional>
#include "curl/curl.h"



using namespace LCUAPI;

LCU_API::~LCU_API() {
}

LCU_API::LCU_API(){
	connected = false;
	host = "127.0.0.1";
	prot[0] = "https://";
	prot[1] = "wss://";
	http_client.setHttp2(true);
}

// 连接LCU
bool LCU_API::Connect() {
	connected = false;
	try {
		if (!auth.GetLeagueClientInfo()) {
			return connected;
		}
		auto request_call = &LCU_API::Request;
		auto obj = this;
		std::string test = "get";
		GET = std::bind(request_call, obj, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, "get");
		POST = std::bind(request_call, obj, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, "post");
		PUT = std::bind(request_call, obj, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, "put");
		DELETe = std::bind(request_call, obj, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, "delete");
		PATCH = std::bind(request_call, obj, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, "patch");
		http_client.setHeader(auth.leagueHeader);
		http_client.setPort(auth.leaguePort);
		// check lol client is ready
		std::string load_ok_str;
		if (!GET(url("/lol-summoner/v1/current-summoner"), load_ok_str, "")) {
			printf("erroe2\n");
			return connected;
		}
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		JSONCPP_STRING err;
		Json::Value root;
		if (!reader->parse(load_ok_str.c_str(), load_ok_str.c_str() + static_cast<int>(load_ok_str.length() - 1), &root, &err)) {
			if (root["httpStatus"].asInt()) {
				return connected;
			}
			else {
				connected = true;
			}
		}
	}
	catch (...) {
		printf("erroe1\n");
		return connected;
	}
	return connected;
}

bool LCU_API::IsAPIServerConnected() {
	return connected;
}

std::string LCU_API::url(const std::string &api) {
	return prot[0] + host + api;
}

bool LCU_API::Request(const std::string& url, std::string& result, const std::string& requestData, const std::string& method) {
	if (!http_client.request(url, result, requestData, method)) {
		connected = false;
		return false;
	}
#ifdef  USE_LOG
	std::cout << "==================" << method << " INFO================" << std::endl;
	std::cout << "API:" << url << std::endl;
	if (!requestData.empty()) {
		std::string content;
#ifdef JSON_FORMAT
		content = utils->formatJson(requestData);
#else
		content = requestData;
#endif // JSON_FORMAT
#ifdef USE_GBK
		content = utils->Utf8ToGbk(content.c_str());
#endif // USE_GBK
		std::cout << "Request Data:" << std::endl << content << std::endl;
	}
	if (!result.empty()) {
		std::string content;
#ifdef JSON_FORMAT
		content = utils->formatJson(result);
#else
		content = result;
#endif // JSON_FORMAT
#ifdef USE_GBK
		content = utils->Utf8ToGbk(content.c_str());
#endif // USE_GBK
		std::cout << "Result:" << std::endl << content << std::endl;
	}
#endif //  USE_LOG
	return true;
}

template<typename T>
bool LCU_API::ResultCheck(const std::string& data, const std::string& attr, T aim) {
	bool ret = false;
	if (!data.empty())
	{
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		JSONCPP_STRING err;
		Json::Value root;
		if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
			try
			{
				if (root[attr].as<T>() == aim) {
					ret = true;
				}
			}
			catch (const std::exception&)
			{
				ret = false;
			}
		}
	}
	return ret;
}

// ==================API==================
bool LCU_API::BuildRoom(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string body = R"({"partyType":"closed","queueId":)" + std::to_string(static_cast<int>(type)) + "}";
	std::string ret;
	POST(url("/lol-lobby/v2/lobby"), ret, body);
	return ResultCheck(ret, "errorCode", NULL);
}

bool LCU_API::BuildTFTNormalRoom() {
	return BuildRoom(QueueID::TFTNormal);
}

bool LCU_API::BuildTFTRankRoom() {
	return BuildRoom(QueueID::TFTRanked);
}

bool LCU_API::ExitRoom() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	DELETe(url("/lol-lobby/v2/lobby"), ret, "");
	return true;
}

Json::Value LCU_API::GetRoom() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-lobby/v2/lobby"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}

	return	Json::nullValue;
}

bool LCU_API::OpenTeam() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	PUT(url("/lol-lobby/v2/lobby/partyType"), ret, R"("open")");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

bool LCU_API::CloseTeam() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	PUT(url("/lol-lobby/v2/lobby/partyType"), ret, R"("closed")");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

bool LCU_API::LobbyAvailability() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	GET(url("/lol-lobby/v1/lobby/availability"), ret, "");
	if (ret == R"("Available")") {
		return true;
	}
	else {
		return false;
	}
}

Json::Value LCU_API::GetInvitations() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-lobby/v1/lobby/invitations"),data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		return root;
	}
	return	Json::nullValue;
}

Json::Value LCU_API::GetGameMode() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-lobby/v1/parties/gamemode"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return	Json::nullValue;
}

bool LCU_API::SetMetaData(PositionPref first, PositionPref second) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* types[] = { "TOP", "JUNGLE", "MIDDLE", "BOTTOM", "UTILITY", "FILL", "UNSELECTED" };
	char const* format = R"({"championSelection" : null,"positionPref" :["%s","%s"] ,"properties" : null,"skinSelection" : null})";
	char data[128];
	sprintf_s(data, 128, format, types[static_cast<int>(first)], types[static_cast<int>(second)]);
	std::string ret;
	PUT(url("/lol-lobby/v1/parties/metadata"), ret, data);
	return true;
}

Json::Value LCU_API::GetPartiedPlayer() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-lobby/v1/parties/player"),data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return	Json::nullValue;
}

bool LCU_API::SetQueue(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	PUT(url("/lol-lobby/v1/parties/queue"), ret, std::to_string(static_cast<int>(type)));
	return true;
}

bool LCU_API::GiveLeaderRole(const char* party_id, const char* puuid) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v1/parties/%s/members/%s/role";
	char url_data[128];
	sprintf_s(url_data, 128, format, party_id, puuid);
	std::string ret;
	PUT(url(url_data),ret, "LEADER");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

Json::Value LCU_API::GetCommsMembers() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-lobby/v2/comms/members"),data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return	Json::nullValue;
}

std::string LCU_API::GetCommsToken() {
	if (!IsAPIServerConnected()) {
		return "";
	}
	std::string ret;
	GET(url("/lol-lobby/v2/comms/token"), ret, "");
	return ret;
}

bool LCU_API::CheckPartyEligibility(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string data;
	POST(url("/lol-lobby/v2/eligibility/party"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			// 解析
			for (auto item : root) {
				if (item["queueId"].asInt() == static_cast<int>(type)) {
					return item["eligible"].asBool();
				}
			}
		}
	}
	return false;
}

bool LCU_API::CheckSelfEligibility(QueueID type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string data;
	POST(url("/lol-lobby/v2/eligibility/self"),data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			// 解析
			for (auto item : root) {
				if (item["queueId"].asInt() == static_cast<int>(type)) {
					return item["eligible"].asBool();
				}
			}
		}
	}
	return false;
}

Json::Value LCU_API::GetLobbyInvitations() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-lobby/v2/lobby/invitations"),data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
			if (root.isArray()) {
				return root;
			}
		}
	}
	return	Json::nullValue;
}

bool LCU_API::InviteBySummonerIdS(std::vector<long long>& ids) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format_out = "[]";
	const char* format_in = R"({"toSummonerId":%I64d},)";
	char row_data[128];
	std::string data = "[";
	for (long long item : ids) {
		sprintf_s(row_data, 128, format_in, item);
		data += row_data;
	}
	data = data.substr(0, data.length() - 1);
	data += "]";
	std::string ret;
	POST(url("/lol-lobby/v2/lobby/invitations"),ret, data);
	if (ret.find("[") != -1) {
		return true;
	}
	return false;
}


bool LCU_API::StartQueue() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	POST(url("/lol-lobby/v2/lobby/matchmaking/search"), ret, "");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

bool LCU_API::StopQueue() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	DELETe(url("/lol-lobby/v2/lobby/matchmaking/search"),ret, "");
	if (ret.empty()) {
		return true;
	}
	else{
		return false;
	}
}

SearchState LCU_API::GetSearchState() {
	if (!IsAPIServerConnected()) {
		return SearchState::Invalid;
	}
	std::string data;
	GET(url("/lol-lobby/v2/lobby/matchmaking/search-state"),data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			std::string state = root["searchState"].asString();

			if (state == "Invalid") {
				return SearchState::Invalid;
			}
			else if (state == "Searching") {
				return SearchState::Searching;
			}
			else if (state == "Found") {
				return SearchState::Found;
			}
		}
	}
	return SearchState::Invalid;
}

Json::Value LCU_API::GetLobbyMembers() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-lobby/v2/lobby/members"),data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			return root;
		}
	}
	return	Json::nullValue;
}

bool LCU_API::SetPositionPreferences(PositionPref first, PositionPref second) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* types[] = { "TOP", "JUNGLE", "MIDDLE", "BOTTOM", "UTILITY", "FILL", "UNSELECTED" };
	const char* format = R"({"firstPreference": "%s","secondPreference": "%s"})";
	char data[128];
	sprintf_s(data, 128, format, types[static_cast<int>(first)], types[static_cast<int>(second)]);
	std::string ret;
	PUT(url("/lol-lobby/v2/lobby/members/localMember/position-preferences"),ret, data);
	if (ret.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::GrantInviteBySummonerId(long long id) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/lobby/members/%I64d/grant-invite";
	char url_buffer[128];
	sprintf_s(url_buffer, 128, format, id);
	std::string ret;
	POST(url(url_buffer),ret, "");
	if (ret == std::to_string(id)) {
		return true;
	}
	return false;
}

bool LCU_API::KickSummonerBySummonerId(long long id) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/lobby/members/%I64d/kick";
	char url_buffer[128];
	sprintf_s(url_buffer, 128, format, id);
	std::string ret;
	POST(url(url_buffer),ret, "");
	if (ret == std::to_string(id)) {
		return true;
	}
	return false;
}

bool LCU_API::PromoteSummonerBySummonerId(long long id) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/lobby/members/%I64d/promote";
	char url_buffer[128];
	sprintf_s(url_buffer, 128, format, id);
	std::string ret;
	POST(url(url_buffer), ret,"");
	if (ret == std::to_string(id)) {
		return true;
	}
	return false;
}

bool LCU_API::RevokeInviteBySummonerId(long long id) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/lobby/members/%I64d/revoke-invite";
	char url_buffer[128];
	sprintf_s(url_buffer, 128, format, id);
	std::string ret;
	POST(url(url_buffer), ret,"");
	if (ret == std::to_string(id)) {
		return true;
	}
	return false;
}

bool LCU_API::SetPartyType(PartyType type) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* types[] = {"open", "closed"};
	std::string ret;
	PUT(url("/lol-lobby/v2/lobby/partyType"),ret, types[static_cast<int>(type)]);
	if (ret.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::IsPartyActive() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	GET(url("/lol-lobby/v2/party-active"),ret, "");
	if (ret == "true") {
		return true;
	}
	return false;
}

bool LCU_API::PlayAgain() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	GET(url("/lol-lobby/v2/play-again"), ret,"");
	if (ret.empty()) {
		return true;
	}
	else {
		return false;
	}
}

Json::Value LCU_API::GetReceivedInvitations() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-lobby/v2/received-invitations"), data,"");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			return root;
		}
	}
	return	Json::nullValue;
}

bool LCU_API::AcceptInvitation(const char* invitationId) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/received-invitations/%s/accept";
	char uri_buffer[128];
	sprintf_s(uri_buffer, 128, format, invitationId);
	std::string data;
	POST(url(uri_buffer), data,"");
	if (data.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::DeclineInvitation(const char* invitationId) {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* format = "/lol-lobby/v2/received-invitations/%s/decline";
	char uri_buffer[128];
	sprintf_s(uri_buffer, 128, format, invitationId);
	std::string data;
	GET(url(uri_buffer), data, "");
	if (data.empty()) {
		return true;
	}
	return false;
}

Json::Value LCU_API::GetAllGridChampions() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-champ-select/v1/all-grid-champions"), data,"");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			return root;
		}
	}
	return	Json::nullValue;
}

Json::Value LCU_API::GetBannableChampionIds() {
	if (!IsAPIServerConnected()) {
		return	Json::nullValue;
	}
	std::string data;
	GET(url("/lol-champ-select/v1/bannable-champion-ids"), data,"");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (root.isArray()) {
			return root;
		}
	}
	return	Json::nullValue;
}

int LCU_API::GetCurrentChampion() {
	if (!IsAPIServerConnected()) {
		return 0;
	}
	std::string data;
	GET(url("/lol-champ-select/v1/current-champion"), data,"");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		return 0;
	}
	return atoi(data.c_str());
}

bool LCU_API::SetGameSettings() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	const char* content = R"({"Accessibility":{"ColorGamma":"0.0000","ColorContrast":"0.0000","ColorBrightness":"0.0000","ColorLevel":"0.0000"},"Voice":{"InputMode":0,"InputDevice":"Default System Device","InputVolume":"0.5000","ActivationSensitivity":"0.6500"},"General":{"CursorScale":"0.0000","SystemMouseSpeed":10,"WindowMode":2,"Height":768,"Width":1024,"WaitForVerticalSync":0,"SnapCameraOnRespawn":0,"ShowTurretRangeIndicators":0,"ShowGodray":0,"RelativeTeamColors":0,"MinimizeCameraMotion":0,"BindSysKeys":0,"HideEyeCandy":1,"EnableTargetedAttackMove":0,"EnableScreenShake":0,"EnableLightFx":0,"EnableCustomAnnouncer":1,"EnableAudio":0,"AutoAcquireTarget":0,"AlwaysShowExtendedTooltip":0},"HUD":{"DrawHealthBars":1,"ShopScale":"0.0000","PracticeToolScale":"0.0000","MinimapScale":"0.0000","KeyboardScrollSpeed":"0.5000","GlobalScale":"0.0000","DeathRecapScale":"0.0000","ShowSummonerNames":0,"EmoteSize":1,"EmotePopupUIDisplayMode":2,"NumericCooldownFormat":3,"ChatScale":0,"SmartCastOnKeyRelease":0,"ShowSummonerNamesInScoreboard":0,"ObjectTooltips":0,"ShowNeutralCamps":0,"ShowHealthBarShake":0,"ShowAttackRadius":0,"ShowAlliedChat":0,"MinimapMoveSelf":0,"FlashScreenWhenStunned":0,"FlashScreenWhenDamaged":0,"EnableLineMissileVis":0,"AutoDisplayTarget":0},"Performance":{"ShadowQuality":0,"FrameCapType":3,"EnvironmentQuality":0,"EffectsQuality":0,"CharacterQuality":0,"EnableHUDAnimations":0,"EnableGrassSwaying":1,"EnableFXAA":0,"CharacterInking":0},"MapSkinOptions":{"MapSkinOptionDisableWorlds":0,"MapSkinOptionDisableURF":0,"MapSkinOptionDisableStarGuardian":0,"MapSkinOptionDisableSnowdown":0,"MapSkinOptionDisableProject":0,"MapSkinOptionDisablePopstar":0,"MapSkinOptionDisablePoolParty":0,"MapSkinOptionDisableOdyssey":0,"MapSkinOptionDisableMSI":0,"MapSkinOptionDisableLunarRevel":0,"MapSkinOptionDisableArcade":0},"Volume":{"VoiceMute":1,"SfxMute":1,"PingsMute":1,"MusicMute":1,"MasterMute":1,"AnnouncerMute":1,"AmbienceMute":1},"FloatingText":{"Special_Enabled":0,"Score_Enabled":0,"QuestReceived_Enabled":0,"ManaDamage_Enabled":0,"Level_Enabled":0,"Invulnerable_Enabled":0,"Heal_Enabled":0,"Gold_Enabled":0,"Experience_Enabled":0,"EnemyDamage_Enabled":0,"Dodge_Enabled":0,"Damage_Enabled":0},"LossOfControl":{"LossOfControlEnabled":0},"Replay":{"EnableDirectedCamera":0},"Highlights":{"VideoQuality":0,"VideoFrameRate":60,"ScaleVideo":720,"AudioQuality":1}})";
	std::string data;
	PATCH(url("/lol-game-settings/v1/game-settings"), data,content);
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return true;
		}
	}
	return false;
}

bool LCU_API::Reconnect() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string data;
	POST(url("/lol-gameflow/v1/reconnect"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return true;
		}
	}
	return false;
}

bool LCU_API::DeclineSearch() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	POST(url("/lol-matchmaking/v1/ready-check/decline"), ret, "");
	if (ret.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::AcceptSearch() {
	if (!IsAPIServerConnected()) {
		return false;
	}
	std::string ret;
	POST(url("/lol-matchmaking/v1/ready-check/accept"), ret, "");
	if (ret.empty()) {
		return true;
	}
	return false;
}

GameFlowPhase LCU_API::GetGameflowPhase() {
	std::string ret;
	printf("getgameflowphase start\n");
	GET(url("/lol-gameflow/v1/gameflow-phase"), ret,"");
	printf("getgameflowphase end\n");
	if (ret == R"("ChampSelect")") {  // 英雄选择
		return GameFlowPhase::ChampSelect;
	}
	else if (ret == R"("InProgress")") {  // 游戏中
		return GameFlowPhase::InProgress;
	}
	else if (ret == R"("EndOfGame")") {  // 结算页面
		return GameFlowPhase::EndOfGame;
	}
	else if (ret == R"("Lobby")") { // 房间中
		return GameFlowPhase::Lobby;
	}
	else if (ret == R"("Reconnect")") {  // 需要重连游戏
		return GameFlowPhase::Reconnect;
	}
	else if (ret == R"("ReadyCheck")") {  // 找到对局了
		return GameFlowPhase::ReadyCheck;
	}
	else if (ret == R"("Matchmaking")") {   // 对局寻找中
		return GameFlowPhase::Matchmaking;
	}
	return GameFlowPhase::None;
}


bool LCU_API::KillUx() {
	std::string ret;
	POST(url("/riotclient/kill-ux"), ret, "");
	if (ret.empty()) {
		return true;
	}
	return false;
}

bool LCU_API::ShowUx() {
	std::string ret;
	POST(url("/riotclient/ux-show"), ret, "");
	if (ret.empty()) {
		return true;
	}
	return false;
}

Json::Value LCU_API::GetCurrentSummoner() {
	std::string data;
	GET(url("/lol-summoner/v1/current-summoner"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return	Json::nullValue;
}

Json::Value LCU_API::GetSummonerByName(const char* name) {
	const char* format = "/lol-summoner/v1/summoners?name=%s";
	char uri[1024];
	std::string content;
	utils->UrlEncode(name, content);
	sprintf_s(uri, format, content.c_str());
	std::string data;
	GET(url(uri), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return	Json::nullValue;
}

Json::Value LCUAPI::LCU_API::GetMatchMakingInfo()
{
	std::string data;
	GET(url("/lol-lobby-team-builder/v1/matchmaking"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return	Json::nullValue;
}

Json::Value LCUAPI::LCU_API::GetMatchMakingInfo2()
{
	std::string data;
	GET(url("/lol-matchmaking/v1/search"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return	Json::nullValue;
}

Json::Value LCUAPI::LCU_API::GetMetadataPlayerStatus()
{
	std::string data;
	GET(url("/lol-gameflow/v1/gameflow-metadata/player-status"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return	Json::nullValue;
}

Json::Value LCU_API::GetEndOfGameData() {
	std::string data;
	GET(url("/lol-end-of-game/v1/gameclient-eog-stats-block"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return	Json::nullValue;
}

Json::Value LCU_API::GetFriendsInfo() {
	std::string data;
	GET(url("/lol-chat/v1/friends"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length()-1), &root, &err)) {
		if (root.isArray()) {
			return root;
		}
	}
	return	Json::nullValue;
}

Json::Value LCUAPI::LCU_API::GetPlayerLootMap()
{
	std::string data;
	GET(url("/lol-loot/v2/player-loot-map"), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return Json::nullValue;
}

Json::Value LCUAPI::LCU_API::GetLootById(const char* id)
{
	const char* format = "/lol-loot/v1/player-loot/%s";
	char uri[512];
	sprintf_s(uri, format, id);
	std::string data;
	GET(url(uri), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return Json::nullValue;
}

Json::Value LCUAPI::LCU_API::CraftLoot(const char* t_loot_name, std::vector<const char*>& r_loot_names, int repeat)
{
	const char* format = "/lol-loot/v1/recipes/%s/craft?repeat=%d";
	char uri[512];
	sprintf_s(uri, format, t_loot_name, repeat);
	Json::Value param;
	for (auto item : r_loot_names) {
		param.append(item);
	}
	std::string data;
	Json::FastWriter writer;
	POST(url(uri), data, writer.write(param).c_str());
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root["httpStatus"].asInt()) {
			return root;
		}
	}
	return Json::nullValue;
}

Json::Value LCUAPI::LCU_API::GetLootContextMenuByLootId(const char* loot_id)
{
	const char* format = "/lol-loot/v1/player-loot/%s/context-menu";
	char uri[512];
	sprintf_s(uri, format, loot_id);
	std::string data;
	POST(url(uri), data, "");
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	JSONCPP_STRING err;
	Json::Value root;
	if (!reader->parse(data.c_str(), data.c_str() + static_cast<int>(data.length() - 1), &root, &err)) {
		if (!root.isObject()) {
			return root;
		}
	}
	return Json::nullValue;
}

bool LCUAPI::LCU_API::IsLootReady()
{
	std::string data;
	GET(url("/lol-loot/v1/ready"), data, "");
	if (data == "true") {
		return true;
	}
	return false;
}
