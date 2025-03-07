#include <stdio.h>
#include "chat_processor.h"
#include "metamod_oslink.h"
#include "schemasystem/schemasystem.h"
#include <networksystem/inetworkserializer.h>
#include <networksystem/inetworkmessages.h>
#include "usermessages.pb.h"

ChatProcessor g_ChatProcessor;
PLUGIN_EXPOSE(ChatProcessor, g_ChatProcessor);
IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CGlobalVars *gpGlobals = nullptr;
IGameEventSystem *g_gameEventSystem = nullptr;

std::map<std::string, std::string> g_vecPhrases;

IUtilsApi* g_pUtils;

ChatProcessorApi* g_pChatProcessorApi = nullptr;
IChatProcessorApi* g_pChatProcessorCore = nullptr;

SH_DECL_HOOK8_void(IGameEventSystem, PostEventAbstract, SH_NOATTRIB, 0, CSplitScreenSlot, bool, int, const uint64*, INetworkMessageInternal*, const CNetMessage*, unsigned long, NetChannelBufType_t)

CGameEntitySystem* GameEntitySystem()
{
	return g_pUtils->GetCGameEntitySystem();
}

void StartupServer()
{
	g_pGameEntitySystem = GameEntitySystem();
	g_pEntitySystem = g_pUtils->GetCEntitySystem();
	gpGlobals = g_pUtils->GetCGlobalVars();
}

void* ChatProcessor::OnMetamodQuery(const char* iface, int* ret)
{
	if (!strcmp(iface, CP_INTERFACE))
	{
		*ret = META_IFACE_OK;
		return g_pChatProcessorCore;
	}

	*ret = META_IFACE_FAILED;
	return nullptr;
}

bool ChatProcessor::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_gameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);

	SH_ADD_HOOK(IGameEventSystem, PostEventAbstract, g_gameEventSystem, SH_MEMBER(this, &ChatProcessor::OnPostEvent), false);

	g_pChatProcessorApi = new ChatProcessorApi();
	g_pChatProcessorCore = g_pChatProcessorApi;

	g_SMAPI->AddListener( this, this );

	return true;
}

std::string Colorizer(std::string str)
{
	for (size_t i = 0; i < std::size(colors_hex); i++)
	{
		size_t pos = 0;

		while ((pos = str.find(colors_text[i], pos)) != std::string::npos)
		{
			str.replace(pos, colors_text[i].length(), colors_hex[i]);
			pos += colors_hex[i].length();
		}
	}

	return str;
}

std::string EscapeString(const std::string &str)
{
	std::string result;
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] == '{' || str[i] == '}')
			result += '\\';
		result += str[i];
	}
	return result;
}

void ChatProcessor::OnPostEvent(CSplitScreenSlot slot, bool bReliable, int nTick, const uint64* pOutPackets, INetworkMessageInternal* pMsg, const CNetMessage* pNetMsg, unsigned long nChannel, NetChannelBufType_t eBufType)
{
	NetMessageInfo_t *info = pMsg->GetNetMessageInfo();

	if(info->m_MessageId == UM_SayText2)
	{
		auto msg = const_cast<CNetMessage*>(pNetMsg)->ToPB<CUserMessageSayText2>();
		int iSlot = msg->entityindex()-1;
		std::string szMessageName = msg->messagename();
		std::string szParam1 = msg->param1();
		std::string szParam2 = msg->param2();
		std::string szName = szParam1;
		std::string szMessage = szParam2;
		std::string szMessageEscape = EscapeString(szMessage);
		bool bBlock = g_pChatProcessorApi->SendHookOnChatMessage(iSlot, szName, szMessageEscape);
		if(bBlock) RETURN_META(MRES_SUPERCEDE);
		char szBuffer[256];
		g_SMAPI->Format(szBuffer, sizeof(szBuffer), g_vecPhrases[szMessageName].c_str(), szName.c_str(), szMessageEscape.c_str());
		msg->set_messagename(Colorizer(szBuffer));
	}

}

bool ChatProcessor::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IGameEventSystem, PostEventAbstract, g_gameEventSystem, SH_MEMBER(this, &ChatProcessor::OnPostEvent), false);
	ConVar_Unregister();
	
	return true;
}

void LoadPhrases()
{
	KeyValues::AutoDelete g_kvPhrases("Phrases");
	char pszPath[256];
	g_SMAPI->Format(pszPath, sizeof(pszPath), "addons/translations/chat_processor.phrases.txt");

	if (!g_kvPhrases->LoadFromFile(g_pFullFileSystem, pszPath))
	{
		Warning("Failed to load %s\n", pszPath);
		return;
	}
	const char* szLanguage = g_pUtils->GetLanguage();
	for (KeyValues *pKey = g_kvPhrases->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey())
		g_vecPhrases[std::string(pKey->GetName())] = std::string(pKey->GetString(szLanguage));
}

void ChatProcessor::AllPluginsLoaded()
{
	char error[64];
	int ret;
	g_pUtils = (IUtilsApi *)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
	{
		g_SMAPI->Format(error, sizeof(error), "Missing Utils system plugin");
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}
	g_pUtils->StartupServer(g_PLID, StartupServer);
	LoadPhrases();
}

///////////////////////////////////////
const char* ChatProcessor::GetLicense()
{
	return "GPL";
}

const char* ChatProcessor::GetVersion()
{
	return "1.0";
}

const char* ChatProcessor::GetDate()
{
	return __DATE__;
}

const char *ChatProcessor::GetLogTag()
{
	return "ChatProcessor";
}

const char* ChatProcessor::GetAuthor()
{
	return "Pisex";
}

const char* ChatProcessor::GetDescription()
{
	return "ChatProcessor";
}

const char* ChatProcessor::GetName()
{
	return "[Chat Processor] Core";
}

const char* ChatProcessor::GetURL()
{
	return "https://discord.gg/g798xERK5Y";
}
