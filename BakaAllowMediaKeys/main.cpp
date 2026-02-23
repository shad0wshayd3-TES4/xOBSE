#include "obse/PluginAPI.h"
#include "obse_common/SafeWrite.h"

//IDebugLog		gLog("BakaAllowMediaKeys.log");
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

extern "C" bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "BakaAllowMediaKeys";
	info->version = 1;

	if (obse->isEditor)
	{
		return false;
	}

	if (obse->obseVersion < OBSE_VERSION_INTEGER)
	{
		return false;
	}

	if (obse->oblivionVersion != OBLIVION_VERSION)
	{
		return false;
	}

	return true;
}

extern "C" bool OBSEPlugin_Load(const OBSEInterface* obse)
{
	g_pluginHandle = obse->GetPluginHandle();

	if (!obse->isEditor)
	{
		SafeWrite8(0x004041E3, 0x0C);
		SafeWrite8(0x004041F2, 0x0D);
	}

	return true;
}
