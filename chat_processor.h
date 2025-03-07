#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include <ISmmPlugin.h>
#include <sh_vector.h>
#include "utlvector.h"
#include "ehandle.h"
#include <iserver.h>
#include <entity2/entitysystem.h>
#include "igameevents.h"
#include "engine/igameeventsystem.h"
#include "vector.h"
#include <deque>
#include <functional>
#include <utlstring.h>
#include <KeyValues.h>
#include "CCSPlayerController.h"
#include "include/menus.h"
#include "include/chat_processor.h"

class ChatProcessor final : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late);
	bool Unload(char* error, size_t maxlen);
	void AllPluginsLoaded();
	void* OnMetamodQuery(const char* iface, int* ret);
private:
	const char* GetAuthor();
	const char* GetName();
	const char* GetDescription();
	const char* GetURL();
	const char* GetLicense();
	const char* GetVersion();
	const char* GetDate();
	const char* GetLogTag();
private:
	void OnPostEvent(CSplitScreenSlot slot, bool bReliable, int nTick, const uint64* pOutPackets, INetworkMessageInternal* pMsg, const CNetMessage* pNetMsg, unsigned long nChannel, NetChannelBufType_t eBufType);
};

class ChatProcessorApi : public IChatProcessorApi
{
public:
	void HookOnChatMessage(SourceMM::PluginId id, OnChatMessageCallback callback) override {
		OnChatMessageHook[id].push_back(callback);
	}
	bool SendHookOnChatMessage(int iSlot, std::string& szName, std::string& szMessage)
	{
		bool bBlock = false;
		for(auto& item : OnChatMessageHook)
		{
			for (auto& callback : item.second) {
				if (callback && !callback(iSlot, szName, szMessage)) {
					bBlock = true;
				}
			}
		}
		return bBlock;
	}
private:
	std::map<int, std::vector<OnChatMessageCallback>> OnChatMessageHook;
};

const std::string colors_text[] = {
    "{DEFAULT}",
    "{WHITE}",
    "{RED}",
    "{LIGHTPURPLE}",
    "{GREEN}",
    "{LIME}",
    "{LIGHTGREEN}",
    "{DARKRED}",
    "{GRAY}",
    "{LIGHTOLIVE}",
    "{OLIVE}",
    "{LIGHTBLUE}",
    "{BLUE}",
    "{PURPLE}",
    "{LIGHTRED}",
    "{GRAYBLUE}",
    "{TEAM}"
};

const std::string colors_hex[] = {
    "\x01",
    "\x01",
    "\x02",
    "\x03",
    "\x04",
    "\x05",
    "\x06",
    "\x07",
    "\x08",
    "\x09",
    "\x10",
    "\x0B",
    "\x0C",
    "\x0E",
    "\x0F",
    "\x0A",
    "\3"
};

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
