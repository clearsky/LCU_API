# LCU_API
LCU API CPP LIBRARY

AUTH HTPP Utils部分代码使用的[KBotExt](https://github.com/KebsCS/KBotExt)

接下来就是体力活了,按照[文档](https://www.mingweisamuel.com/lcu-schema/tool/#/)把LCU API调用实现

api_record.txt记录了尝试过的api的功能,但有些api实在看不懂什么意思,也许后面的api能让前面的api变得清晰

API实在太多了,文档又很有限,只能一个个尝试

有些比较鸡肋的api,没有实现调用,比如自定义自动加机器人之类的,有需要的可以自己实现

有兴趣的可以加入QQ群:742181447,一起交流

## 编译
默认是编译成Lib,如果要编译成DLL需要自己手动配置需要配置的东西。当然建议不管编译成什么,都最好自己手动配置,结构比较简单,配置起来也容易。

唯一比较麻烦的就是cpprest安装,建议用vcpkg,因为cpprest还有一堆依赖,用vcpkg会极其方便,自己下载源码编译出问题了会很痛苦。

## LCU_API:
持续更新中...
|Name|API|Describe
|------|------|------|
|Connect||连接LCU,websocket初始化,事件绑定等|
|HandleEventMessage||处理websocket消息|
|BindEvent||订阅websocket api事件|
|UnBindEvent||退订websocket api事件|
|AddEventFilter||增加过滤条件,FILTER模式下订阅了所有websocket api事件,通过filter过滤处理|
|RemoveEventFilter||删除过滤条件|
|IsAPIServerConnected||是否连接到了LCU|
|ResultCheck||检查API是否调用成功|
|url||把uri拼接成url|
|Request||HTTP请求|
|BindEventAuto||对于OnEvent接口的自动订阅|
|OnCreateRoom||给创建房间事件添加一个回调函数|
|OnCloseRoom||给退出房间事件添加一个回调函数|
|OnUpdateRoom||给房间更新事件添加一个回调函数|
|BuildRoom|/lol-lobby/v2/lobby|创建房间|
|BuildTFTNormalRoom|/lol-lobby/v2/lobby|创建云顶匹配房间|
|BuildTFTRankRoom|/lol-lobby/v2/lobby|创建排位房间|
|ExitRoom|/lol-lobby/v2/lobby|退出房间|
|GetRoom|/lol-lobby/v2/lobby|获取当前房间信息|
|OpenTeam|/lol-lobby/v2/lobby/partyType|打开公开的小队|
|CloseTeam|/lol-lobby/v2/lobby/partyType|关闭公开的小队|
|LobbyAvailability|/lol-lobby/v1/lobby/availability|判断是否在房间中|
|GetInvitations|/lol-lobby/v1/lobby/invitations|获取自定义房间右下角的邀请信息|
|GetGameMode|/lol-lobby/v1/parties/gamemode|获取当前房间的游戏模式信息|
|SetMetaData|/lol-lobby/v1/parties/metadata|主要用于排位的时候选位置|
|GetPartiedPlayer|/lol-lobby/v1/parties/player|获取房间玩家的信息,还包含其他信息,在房间外也可以获取,一大堆信息,不止playerinfo|
|SetQueue|/lol-lobby/v1/parties/queue|切换房间模式|
|GiveLeaderRole|/lol-lobby/v1/parties/{partyId}/members/{puuid}/role|转移房主|
|GetCommsMembers|lol-lobby/v2/comms/members |获取房间内可聊天的playeri信息,比GetPartiedPlayer更轻量|
|GetCommsToken|/lol-lobby/v2/comms/token|获取comm toke|
|CheckPartyEligibility|/lol-lobby/v2/eligibility/party|查询当前房间是否有资格转换到其他类型的房间,和self版本不同,同一个房间中可能有拖后腿的导致不能换到某些模式|
|CheckSelfEligibility|/lol-lobby/v2/eligibility/self|自己能否创建某种类型的房间|
|GetLobbyInvitations|/lol-lobby/v2/lobby/invitations|房间右下角邀请信息|
|InviteBySummonerIdS|/lol-lobby/v2/lobby/invitations|邀请|
|StartQueue|/lol-lobby/v2/lobby/matchmaking/search|POST开始匹配|
|StopQueue|/lol-lobby/v2/lobby/matchmaking/search|DELETE终止匹配|
|GetSearchState|/lol-lobby/v2/lobby/matchmaking/search-state|获取匹配状态|
|GetLobbyMembers|/lol-lobby/v2/lobby/members|获取房间内玩家信息|
|SetPositionPreferences|/lol-lobby/v2/lobby/members/localMember/position-preferences|设置排位预选位|
|GrantInviteBySummonerId|/lol-lobby/v2/lobby/members/{summoner ID}/grant-invite|授予邀请权限|
|KickSummonerBySummonerId|/lol-lobby/v2/lobby/members/{summoner ID}/kick|踢出房间|
|PromoteSummonerBySummonerId|/lol-lobby/v2/lobby/members/{summoner ID}/promote|转移房主权限|
|RevokeInviteBySummonerId|/lol-lobby/v2/lobby/members/{summoner ID}/revoke-invite|收回邀请权限|
|SetPartyType|/lol-lobby/v2/lobby/partyType|设置小队状态|
|IsPartyActive|/lol-lobby/v2/party-active|是否在房间中|
|PlayAgain|/lol-lobby/v2/play-again|再玩一次|
|GetReceivedInvitations|/lol-lobby/v2/received-invitations|被邀请信息|
|AcceptInvitation|/lol-lobby/v2/received-invitations/{invitationId}/accept|接受邀请|
|DeclineInvitation|/lol-lobby/v2/received-invitations/{invitationId}/decline|拒绝邀请|
|GetAllGridChampions|/lol-champ-select/v1/all-grid-champions|获取所有英雄信息|
|GetBannableChampionIds|/lol-champ-select/v1/bannable-champion-ids|可以ban的英雄列表|
|GetCurrentChampion|/lol-champ-select/v1/current-champion|当前选择的英雄ID|
