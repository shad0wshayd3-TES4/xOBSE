#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"
#include "obse/GameAPI.h"
#include "obse/GameObjects.h"
#include "obse/ParamInfos.h"
#include "obse/Script.h"
#include "obse_common/SafeWrite.h"
#include <string>

#include "glaze/glaze.hpp"

IDebugLog		gLog("BakaRemasteredLeveling.log");
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

namespace Prefs
{
	struct ISettings
	{
		bool bBaseHealthFormula{ true };
		double fBaseHealthStrengthMult{ 0.6666 };
		double fBaseHealthEnduranceMult{ 1.3333 };
		double fBaseHealthEnduranceLevelMult{ 0.1 };
		
		bool bBaseMagickaFormula{ true };
		double fBaseMagickaIntelligenceMult{ 2.0 };

		bool bBaseFatigueFormula{ true };
		double fBaseFatigueWillpowerMult{ 2.6666 };
		double fBaseFatigueAgilityMult{ 1.3333 };

		bool bHealthRegenFormula{ true };
		double fHealthRegenEnduranceMult{ 0.34 };
		double fHealthRegenBase{ 0.16 };
		double fHealthRegenOutOfCombatMult{ 6.0 };

		bool bMagickaRegenFormula{ true };
		double fMagickaRegenWillpowerQuadMult{ 0.0003 };
		double fMagickaRegenWillpowerLinearMult{ 0.01 };
		double fMagickaRegenOutOfCombatMult{ 2.0 };

		bool bFatigueRegenFormula{ true };
		double fFatigueRegenAgilityMult{ 4.0 };
		double fFatigueRegenBase{ 6.0 };
		double fFatigueRegenOutOfCombatMult{ 2.0 };

		bool bFatigueRegenDelay{ true };
		bool bFatigueRegenDelayMovement{ false };
		double fFatigueRegenDelayTime{ 2.0 };
	};
	static ISettings Settings{};

	static void Load()
	{
		auto path = std::filesystem::path{ "Data/OBSE/plugins/BakaRemasteredLeveling.json" };
		if (std::filesystem::exists(path))
		{
			auto ec = glz::read_file_json(Settings, path.string(), std::string{});
			if (!ec)
				return;
			// error
		}
	}
}

namespace Hooks
{
	namespace Regen
	{
		static float fFatigueDelay{ 0.0f };
	}

	namespace hkCalcPCBaseHealth
	{
		static std::int32_t New_CalcPCBaseHealth(std::int32_t a_str, std::int32_t a_end, std::int32_t a_lvl)
		{
			auto m_str = a_str * Prefs::Settings.fBaseHealthStrengthMult;
			auto m_end = a_end * Prefs::Settings.fBaseHealthEnduranceMult;
			auto m_lvl = a_end * (a_lvl - 1) * Prefs::Settings.fBaseHealthEnduranceLevelMult;
			return floor(m_str + m_end + m_lvl);
		}

		namespace NPC_
		{
			static constexpr std::uint32_t HookAddr{ 0x00522624 };
			static constexpr std::uint32_t RetnAddr{ 0x00522684 };

			static std::int32_t __fastcall CalcPCBaseHealth(TESNPC* a_npc)
			{
				auto str = a_npc->GetActorValue(kActorVal_Strength);
				auto end = a_npc->GetActorValue(kActorVal_Endurance);
				auto lvl = a_npc->actorBaseData.level;
				return New_CalcPCBaseHealth(str, end, lvl);
			}

			static __declspec(naked) void Hook(void)
			{
				__asm
				{
					add		esp, 4
					mov		ecx, edi
					call	CalcPCBaseHealth
					jmp		RetnAddr
				}
			}
		}

		namespace ACHR
		{
			static constexpr std::uint32_t HookAddr{ 0x005E2266 };
			static constexpr std::uint32_t RetnAddr{ 0x005E226B };

			static std::int32_t __fastcall CalcPCBaseHealth(Actor* a_actor)
			{
				auto str = a_actor->GetActorValue(kActorVal_Strength);
				auto end = a_actor->GetActorValue(kActorVal_Endurance);
				auto lvl = 1;
				if (auto actorBase = (TESActorBase*)Oblivion_DynamicCast(a_actor->GetBaseForm(), 0, RTTI_TESForm, RTTI_TESActorBase, 0))
					lvl = actorBase->actorBaseData.level;
				return New_CalcPCBaseHealth(str, end, lvl);
			}

			static __declspec(naked) void Hook(void)
			{
				__asm
				{
					mov		ecx, esi
					call	CalcPCBaseHealth
					jmp		RetnAddr
				}
			}
		}

		static void Install()
		{
			WriteRelJump(NPC_::HookAddr, reinterpret_cast<std::uint32_t>(NPC_::Hook)); // TESNPC::InitValues
			WriteRelJump(ACHR::HookAddr, reinterpret_cast<std::uint32_t>(ACHR::Hook)); // Actor::GetDerivedAttributeValues
		}
	}

	namespace hkCalculateMagicka
	{
		static std::int32_t New_CalculateMagicka(std::int32_t a_int)
		{
			auto m_int = a_int * Prefs::Settings.fBaseMagickaIntelligenceMult;
			return floor(m_int);
		}

		namespace NPC_
		{
			static constexpr std::uint32_t HookAddr{ 0x00522693 };
			static constexpr std::uint32_t RetnAddr{ 0x005226AA };

			static std::int32_t __fastcall CalculateMagicka(TESNPC* a_npc)
			{
				auto inte = a_npc->GetActorValue(kActorVal_Intelligence);
				return New_CalculateMagicka(inte);
			}

			static __declspec(naked) void Hook(void)
			{
				__asm
				{
					mov		ecx, edi
					call	CalculateMagicka
					jmp		RetnAddr
				}
			}
		}

		namespace ACHR
		{
			static constexpr std::uint32_t HookAddr{ 0x005E22B5 };
			static constexpr std::uint32_t RetnAddr{ 0x005E22BA };

			static std::int32_t __fastcall CalculateMagicka(Actor* a_actor)
			{
				auto inte = a_actor->GetActorValue(kActorVal_Intelligence);
				return New_CalculateMagicka(inte);
			}

			static __declspec(naked) void Hook(void)
			{
				__asm
				{
					mov		ecx, esi
					call	CalculateMagicka
					jmp		RetnAddr
				}
			}
		}

		static void Install()
		{
			WriteRelJump(NPC_::HookAddr, reinterpret_cast<std::uint32_t>(NPC_::Hook)); // TESNPC::InitValues
			WriteRelJump(ACHR::HookAddr, reinterpret_cast<std::uint32_t>(ACHR::Hook)); // Actor::GetDerivedAttributeValues
		}
	}

	namespace hkCalculateFatigue
	{
		static std::int32_t New_CalculateFatigue(std::int32_t a_wil, std::int32_t a_agl)
		{
			auto m_wil = a_wil * Prefs::Settings.fBaseFatigueWillpowerMult;
			auto m_agl = a_agl * Prefs::Settings.fBaseFatigueAgilityMult;
			return floor(m_wil + m_agl);
		}

		namespace NPC_
		{
			static constexpr std::uint32_t HookAddr{ 0x005226B3 };
			static constexpr std::uint32_t RetnAddr{ 0x005226FD };

			static std::int32_t __fastcall CalculateFatigue(TESNPC* a_npc)
			{
				auto wil = a_npc->GetActorValue(kActorVal_Willpower);
				auto agl = a_npc->GetActorValue(kActorVal_Agility);
				return New_CalculateFatigue(wil, agl);
			}

			static __declspec(naked) void Hook(void)
			{
				__asm
				{
					mov		ecx, edi
					call	CalculateFatigue
					jmp		RetnAddr
				}
			}
		}

		namespace ACHR
		{
			static constexpr std::uint32_t HookAddr{ 0x005E2309 };
			static constexpr std::uint32_t RetnAddr{ 0x005E230E };

			static std::int32_t __fastcall CalculateFatigue(Actor* a_actor)
			{
				auto wil = a_actor->GetActorValue(kActorVal_Willpower);
				auto agl = a_actor->GetActorValue(kActorVal_Agility);
				return New_CalculateFatigue(wil, agl);
			}

			static __declspec(naked) void Hook(void)
			{
				__asm
				{
					mov		ecx, esi
					call	CalculateFatigue
					jmp		RetnAddr
				}
			}
		}

		static void Install()
		{
			WriteRelJump(NPC_::HookAddr, reinterpret_cast<std::uint32_t>(NPC_::Hook)); // TESNPC::InitValues
			WriteRelJump(ACHR::HookAddr, reinterpret_cast<std::uint32_t>(ACHR::Hook)); // Actor::GetDerivedAttributeValues
		}
	}

	namespace hkCalcHealthReturn
	{
		static float New_CalcHealthReturn(std::int32_t a_end)
		{
			auto m_end = a_end * Prefs::Settings.fHealthRegenEnduranceMult;
			auto c_hea = (m_end / 100.0) + Prefs::Settings.fHealthRegenBase;
			return static_cast<float>(c_hea);
		}

		namespace
		{
			static constexpr std::uint32_t HookAddr0{ 0x005FAE66 };
			static constexpr std::uint32_t HookAddr1{ 0x0065F85C };
			static constexpr std::uint32_t HookAddr2{ 0x0066F93C };

			static void __fastcall RestoreHealth(Actor* a_this, float a_secondsElapsed)
			{
				if (a_this->refID != 0x00000014)
					return;

				auto curr = a_this->GetAV_F(kActorVal_Health);
				auto base = a_this->GetBaseActorValue(kActorVal_Health) + a_this->GetAVModifier(eAVModifier::kAVModifier_Max, kActorVal_Health);
				if (curr < base)
				{
					auto end = a_this->GetActorValue(kActorVal_Endurance);
					auto reg = New_CalcHealthReturn(end) * a_secondsElapsed;
					if (reg > 0.0f)
					{
						if (!a_this->IsInCombat(false))
							reg *= Prefs::Settings.fHealthRegenOutOfCombatMult;
						a_this->DamageAV_F(kActorVal_Health, reg, 0);
					}
				}
			}
		}

		static void Install()
		{
			static constexpr std::uint32_t addr{ 0x005FAE4E };
			static constexpr std::uint32_t retn{ 0x005FAE5C };
			static constexpr std::uint32_t fill{ retn - addr };
			WriteNop(addr, fill); // Actor::UpdateTimedSystems, nop PlayerCharacter::IsSleeping check

			WriteRelCall(HookAddr0, reinterpret_cast<std::uintptr_t>(RestoreHealth)); // Actor::UpdateTimedSystems
			WriteRelCall(HookAddr1, reinterpret_cast<std::uintptr_t>(RestoreHealth)); // PlayerCharacter::PlayerSleep
			WriteRelCall(HookAddr2, reinterpret_cast<std::uintptr_t>(RestoreHealth)); // PlayerCharacter::FastTravel
		}
	}

	namespace hkCalcMagickaReturn
	{
		static float New_CalcMagickaReturn(std::int32_t a_wil, std::int32_t a_stunted)
		{
			if (a_stunted > 0)
				return 0.0f;
			auto q_wil = a_wil * a_wil * Prefs::Settings.fMagickaRegenWillpowerQuadMult;
			auto m_wil = a_wil * Prefs::Settings.fMagickaRegenWillpowerLinearMult;
			auto c_mag = q_wil + m_wil;
			return static_cast<float>(c_mag);
		}

		namespace
		{
			static constexpr std::uint32_t HookAddr0{ 0x0062B406 };
			static constexpr std::uint32_t HookAddr1{ 0x0062B54C };
			static constexpr std::uint32_t HookAddr2{ 0x0065F873 };
			static constexpr std::uint32_t HookAddr3{ 0x0066F953 };
			static constexpr std::uint32_t HookAddr4{ 0x00678038 };
			static constexpr std::uint32_t HookAddr5{ 0x005FAE49 };

			static void __fastcall RestoreMagicka(Actor* a_this, float a_secondsElapsed, bool a_ignoreSpell)
			{
				if (a_this->magicCaster.GetActiveMagicItem() && a_ignoreSpell)
					return;

				auto curr = a_this->GetAV_F(kActorVal_Magicka);
				auto base = a_this->GetBaseActorValue(kActorVal_Magicka) + a_this->GetAVModifier(eAVModifier::kAVModifier_Max, kActorVal_Magicka);
				if (curr < base)
				{
					auto wil = a_this->GetActorValue(kActorVal_Willpower);
					auto stn = a_this->GetActorValue(kActorVal_StuntedMagicka);
					auto reg = New_CalcMagickaReturn(wil, stn) * a_secondsElapsed;
					if (reg > 0.0f)
					{
						if (!a_this->IsInCombat(false))
							reg *= Prefs::Settings.fMagickaRegenOutOfCombatMult;
						a_this->DamageAV_F(kActorVal_Magicka, reg, 0);
					}
				}
			}

			static void __fastcall UpdateCastPowers(Actor* a_this, float a_secondsElapsed)
			{
				RestoreMagicka(a_this, a_secondsElapsed, true);
				a_this->UpdateCastPowers(a_secondsElapsed);
			}
		}

		static void Install()
		{
			WriteRelCall(HookAddr0, reinterpret_cast<std::uintptr_t>(RestoreMagicka)); // HighProcess::ProcessAim
			WriteRelCall(HookAddr1, reinterpret_cast<std::uintptr_t>(RestoreMagicka)); // HighProcess::ProcessCast
			WriteRelCall(HookAddr2, reinterpret_cast<std::uintptr_t>(RestoreMagicka)); // PlayerCharacter::PlayerSleep
			WriteRelCall(HookAddr3, reinterpret_cast<std::uintptr_t>(RestoreMagicka)); // PlayerCharacter::FastTravel
			WriteRelCall(HookAddr4, reinterpret_cast<std::uintptr_t>(RestoreMagicka)); // ProcessLists::UpdateHighList

			static constexpr std::uint32_t addr{ 0x005FAD2A };
			static constexpr std::uint32_t retn{ 0x005FAE3F };
			static constexpr std::uint32_t fill{ retn - addr };
			WriteNop(addr, fill); // Actor::UpdateTimedSystems, nop inlined RestoreMagicka

			WriteRelCall(HookAddr5, reinterpret_cast<std::uintptr_t>(UpdateCastPowers)); // Actor::UpdateTimedSystems
		}
	}

	namespace hkCalcFatigueReturn
	{
		static float New_CalcFatigueReturn(std::int32_t a_agl)
		{
			auto m_agl = a_agl * Prefs::Settings.fFatigueRegenAgilityMult;
			auto c_fat = (m_agl / 100.0) + Prefs::Settings.fFatigueRegenBase;
			return static_cast<float>(c_fat);
		}

		namespace
		{
			static constexpr std::uint32_t HookAddr0{ 0x005FAD25 };
			static constexpr std::uint32_t HookAddr1{ 0x0065F888 };
			static constexpr std::uint32_t HookAddr2{ 0x0066F968 };
			static constexpr std::uint32_t HookAddr3{ 0x00678049 };

			static void __fastcall RestoreFatigue(Actor* a_this, float a_secondsElapsed)
			{
				if (Prefs::Settings.bFatigueRegenDelay && a_this->refID == 0x00000014)
				{
					Regen::fFatigueDelay -= a_secondsElapsed;
					if (Regen::fFatigueDelay > 0.0f)
						return;
				}

				auto curr = a_this->GetAV_F(kActorVal_Fatigue);
				auto base = a_this->GetBaseActorValue(kActorVal_Fatigue) + a_this->GetAVModifier(eAVModifier::kAVModifier_Max, kActorVal_Fatigue);
				if (curr < base)
				{
					auto agl = a_this->GetActorValue(kActorVal_Agility);
					auto reg = New_CalcFatigueReturn(agl) * a_secondsElapsed;
					if (reg > 0.0f)
					{
						if (!a_this->IsInCombat(false))
							reg *= Prefs::Settings.fFatigueRegenOutOfCombatMult;
						a_this->DamageAV_F(kActorVal_Fatigue, reg, 0);
					}
				}
			}
		}

		static void Install()
		{
			WriteRelCall(HookAddr0, reinterpret_cast<std::uint32_t>(RestoreFatigue)); // Actor::UpdateTimedSystems
			WriteRelCall(HookAddr1, reinterpret_cast<std::uint32_t>(RestoreFatigue)); // PlayerCharacter::PlayerSleep
			WriteRelCall(HookAddr2, reinterpret_cast<std::uint32_t>(RestoreFatigue)); // PlayerCharacter::FastTravel
			WriteRelCall(HookAddr3, reinterpret_cast<std::uint32_t>(RestoreFatigue)); // ProcessLists::UpdateHighList
		}
	}

	namespace hkDrainFatigue
	{
		namespace
		{
			static constexpr std::uint32_t HookAddr0{ 0x005E4075 };
			static constexpr std::uint32_t HookAddr1{ 0x005F5C5D };
			static constexpr std::uint32_t HookAddr2{ 0x005FACBF };
			static constexpr std::uint32_t HookAddr3{ 0x005FAD16 };
			static constexpr std::uint32_t HookAddr4{ 0x005FCA6F };
			static constexpr std::uint32_t HookAddr5{ 0x005FD4D1 };
			static constexpr std::uint32_t HookAddr6{ 0x00672B7C };

			static void __fastcall DrainFatigue(Actor* a_this, float a_amount)
			{
				a_this->DrainFatigue(a_amount);
				if (a_this->refID == 0x00000014)
					Regen::fFatigueDelay = Prefs::Settings.fFatigueRegenDelayTime;
			}
		}

		static void Install()
		{
			WriteRelCall(HookAddr0, reinterpret_cast<std::uint32_t>(DrainFatigue)); // Actor::CheckDrainFatigueAttacking	-- Attack
			WriteRelCall(HookAddr1, reinterpret_cast<std::uint32_t>(DrainFatigue)); // Actor::HandleBlockedAttack			-- Block
			WriteRelCall(HookAddr3, reinterpret_cast<std::uint32_t>(DrainFatigue)); // Actor::UpdateTimedSystems			-- Drawn Bow
			WriteRelCall(HookAddr4, reinterpret_cast<std::uint32_t>(DrainFatigue)); // MagicCaster::CastSpell?				-- Cast
			WriteRelCall(HookAddr5, reinterpret_cast<std::uint32_t>(DrainFatigue)); // Actor::PickAnimations				-- Drawn Bow?
			
			if (!Prefs::Settings.bFatigueRegenDelayMovement)
				return;
			WriteRelCall(HookAddr2, reinterpret_cast<std::uint32_t>(DrainFatigue)); // Actor::UpdateTimedSystems			-- Run
			WriteRelCall(HookAddr6, reinterpret_cast<std::uint32_t>(DrainFatigue)); // PlayerCharacter::Update				-- Jump
		}
	}

	static void Install()
	{
		if (Prefs::Settings.bBaseHealthFormula)
			Hooks::hkCalcPCBaseHealth::Install();
		if (Prefs::Settings.bBaseMagickaFormula)
			Hooks::hkCalculateMagicka::Install();
		if (Prefs::Settings.bBaseFatigueFormula)
			Hooks::hkCalculateFatigue::Install();
		if (Prefs::Settings.bHealthRegenFormula)
			Hooks::hkCalcHealthReturn::Install();
		if (Prefs::Settings.bMagickaRegenFormula)
			Hooks::hkCalcMagickaReturn::Install();
		if (Prefs::Settings.bFatigueRegenFormula)
			Hooks::hkCalcFatigueReturn::Install();
		if (Prefs::Settings.bFatigueRegenDelay)
			Hooks::hkDrainFatigue::Install();
	}
}

namespace
{
	void MessageCallback(OBSEMessagingInterface::Message* a_msg)
	{
		switch (a_msg->type)
		{
		case OBSEMessagingInterface::kMessage_PostLoad:
			Prefs::Load();
			Hooks::Install();
			break;
		default:
			break;
		}
	}
}

extern "C" bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
{
	_MESSAGE("BakaRemasteredLeveling: Query");

	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "BakaRemasteredLeveling";
	info->version = 1;

	if (obse->isEditor)
	{
		return false;
	}

	if (obse->obseVersion < OBSE_VERSION_INTEGER)
	{
		_ERROR("OBSE version too old (got %u expected at least %u)", obse->obseVersion, OBSE_VERSION_INTEGER);
		return false;
	}

	if (obse->oblivionVersion != OBLIVION_VERSION)
	{
		_ERROR("incorrect Oblivion version (got %08X need %08X)", obse->oblivionVersion, OBLIVION_VERSION);
		return false;
	}

	return true;
}

extern "C" bool OBSEPlugin_Load(const OBSEInterface* obse)
{
	_MESSAGE("BakaRemasteredLeveling: Load");

	g_pluginHandle = obse->GetPluginHandle();

	const auto messaging = (OBSEMessagingInterface*)obse->QueryInterface(kInterface_Messaging);
	if (!messaging)
		return false;
	messaging->RegisterListener(g_pluginHandle, "OBSE", MessageCallback);

	return true;
}
