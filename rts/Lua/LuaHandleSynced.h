/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_HANDLE_SYNCED
#define LUA_HANDLE_SYNCED

#include <string>

using std::string;

#include "LuaHandle.h"
#include "LuaRulesParams.h"
#include "System/UnorderedMap.hpp"

struct lua_State;
class LuaSyncedCtrl;
class CLuaHandleSynced;
struct BuildInfo;


class CUnsyncedLuaHandle : public CLuaHandle
{
	friend class CLuaHandleSynced;

	public: // call-ins
		bool DrawUnit(const CUnit* unit) override;
		bool DrawFeature(const CFeature* feature) override;
		bool DrawShield(const CUnit* unit, const CWeapon* weapon) override;
		bool DrawProjectile(const CProjectile* projectile) override;

	public: // all non-eventhandler callins
		void RecvFromSynced(lua_State* srcState, int args); // not an engine call-in

	protected:
		CUnsyncedLuaHandle(CLuaHandleSynced* base, const string& name, int order);
		virtual ~CUnsyncedLuaHandle();

		bool Init(const string& code, const string& file);

		static CUnsyncedLuaHandle* GetUnsyncedHandle(lua_State* L) {
			assert(dynamic_cast<CUnsyncedLuaHandle*>(CLuaHandle::GetHandle(L)) != nullptr);
			return static_cast<CUnsyncedLuaHandle*>(CLuaHandle::GetHandle(L));
		}

	protected:
		CLuaHandleSynced& base;
};



class CSyncedLuaHandle : public CLuaHandle
{
	friend class CLuaHandleSynced;

	public: // call-ins
		bool CommandFallback(const CUnit* unit, const Command& cmd) override;
		bool AllowCommand(const CUnit* unit, const Command& cmd, bool fromSynced) override;

		bool AllowUnitCreation(const UnitDef* unitDef, const CUnit* builder, const BuildInfo* buildInfo) override;
		bool AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture) override;
		bool AllowUnitBuildStep(const CUnit* builder, const CUnit* unit, float part) override;
		bool AllowUnitTransport(const CUnit* transporter, const CUnit* transportee) override;
		bool AllowUnitCloak(const CUnit* unit, const CUnit* enemy) override;
		bool AllowUnitDecloak(const CUnit* unit, const CSolidObject* object, const CWeapon* weapon) override;
		bool AllowFeatureCreation(const FeatureDef* featureDef, int allyTeamID, const float3& pos) override;
		bool AllowFeatureBuildStep(const CUnit* builder, const CFeature* feature, float part) override;
		bool AllowResourceLevel(int teamID, const string& type, float level) override;
		bool AllowResourceTransfer(int oldTeam, int newTeam, const char* type, float amount) override;
		bool AllowDirectUnitControl(int playerID, const CUnit* unit) override;
		bool AllowBuilderHoldFire(const CUnit* unit, int action) override;
		bool AllowStartPosition(int playerID, int teamID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos) override;

		bool TerraformComplete(const CUnit* unit, const CUnit* build) override;
		bool MoveCtrlNotify(const CUnit* unit, int data) override;

		int AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID) override;
		bool AllowWeaponTarget(
			unsigned int attackerID,
			unsigned int targetID,
			unsigned int attackerWeaponNum,
			unsigned int attackerWeaponDefID,
			float* targetPriority
		) override;
		bool AllowWeaponInterceptTarget(const CUnit* interceptorUnit, const CWeapon* interceptorWeapon, const CProjectile* interceptorTarget) override;

		bool UnitPreDamaged(
			const CUnit* unit,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			bool paralyzer,
			float* newDamage,
			float* impulseMult
		) override;

		bool FeaturePreDamaged(
			const CFeature* feature,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			float* newDamage,
			float* impulseMult
		) override;

		bool ShieldPreDamaged(
			const CProjectile* projectile,
			const CWeapon* shieldEmitter,
			const CUnit* shieldCarrier,
			bool bounceProjectile,
			const CWeapon* beamEmitter,
			const CUnit* beamCarrier,
			const float3& startPos,
			const float3& hitPos
		) override;

		bool SyncedActionFallback(const string& line, int playerID) override;

	protected:
		CSyncedLuaHandle(CLuaHandleSynced* base, const string& name, int order);
		virtual ~CSyncedLuaHandle();

		bool Init(const string& code, const string& file);

		static CSyncedLuaHandle* GetSyncedHandle(lua_State* L) {
			assert(dynamic_cast<CSyncedLuaHandle*>(CLuaHandle::GetHandle(L)));
			return static_cast<CSyncedLuaHandle*>(CLuaHandle::GetHandle(L));
		}

	protected:
		CLuaHandleSynced& base;

		spring::unordered_map<string, string> textCommands; // name, help

	private:
		int origNextRef;

	private: // call-outs
		static int SyncedRandom(lua_State* L);
		static int SyncedRandomSeed(lua_State* L);

		static int SyncedNext(lua_State* L);
		static int SyncedPairs(lua_State* L);

		static int SendToUnsynced(lua_State* L);

		static int AddSyncedActionFallback(lua_State* L);
		static int RemoveSyncedActionFallback(lua_State* L);

		static int GetWatchUnitDef(lua_State* L);
		static int SetWatchUnitDef(lua_State* L);
		static int GetWatchFeatureDef(lua_State* L);
		static int SetWatchFeatureDef(lua_State* L);
		static int GetWatchWeaponDef(lua_State* L);
		static int SetWatchWeaponDef(lua_State* L);
};


class CLuaHandleSynced
{
	public: // Non-eventhandler call-ins
		bool GotChatMsg(const string& msg, int playerID) {
			return syncedLuaHandle.GotChatMsg(msg, playerID) || unsyncedLuaHandle.GotChatMsg(msg, playerID);
		}

		bool RecvLuaMsg(const string& msg, int playerID) {
			return syncedLuaHandle.RecvLuaMsg(msg, playerID);
		}

	public:
		void CheckStack() {
			syncedLuaHandle.CheckStack();
			unsyncedLuaHandle.CheckStack();
		}

		static CUnsyncedLuaHandle* GetUnsyncedHandle(lua_State* L) {
			if (!CLuaHandle::GetHandleSynced(L))
				return CUnsyncedLuaHandle::GetUnsyncedHandle(L);

			auto slh = CSyncedLuaHandle::GetSyncedHandle(L);
			return &slh->base.unsyncedLuaHandle;
		}

		static CSyncedLuaHandle* GetSyncedHandle(lua_State* L) {
			if (CLuaHandle::GetHandleSynced(L))
				return CSyncedLuaHandle::GetSyncedHandle(L);

			auto ulh = CUnsyncedLuaHandle::GetUnsyncedHandle(L);
			return &ulh->base.syncedLuaHandle;
		}

	protected:
		CLuaHandleSynced(const string& name, int order);
		virtual ~CLuaHandleSynced();

		string LoadFile(const string& filename, const string& modes) const;
		void Init(const string& syncedFile, const string& unsyncedFile, const string& modes);

		bool IsValid() const {
			return (syncedLuaHandle.IsValid() && unsyncedLuaHandle.IsValid());
		}
		void KillLua(bool inFreeHandler = false) {
			syncedLuaHandle.KillLua(inFreeHandler);
			unsyncedLuaHandle.KillLua(inFreeHandler);
		}

	#define SET_PERMISSION(name, type) \
		void Set ## name(const type arg) { \
			syncedLuaHandle.Set ## name(arg); \
			unsyncedLuaHandle.Set ## name(arg); \
		}

		SET_PERMISSION(FullCtrl, bool);
		SET_PERMISSION(FullRead, bool);
		SET_PERMISSION(CtrlTeam, int);
		SET_PERMISSION(ReadTeam, int);
		SET_PERMISSION(ReadAllyTeam, int);
		SET_PERMISSION(SelectTeam, int);

	#undef SET_PERMISSION

	protected:
		friend class CUnsyncedLuaHandle;
		friend class CSyncedLuaHandle;

		// hooks to add code during initialization
		virtual bool AddSyncedCode(lua_State* L) = 0;
		virtual bool AddUnsyncedCode(lua_State* L) = 0;

		// call-outs
		static int LoadStringData(lua_State* L);
		static int CallAsTeam(lua_State* L);

	public:
		CSyncedLuaHandle syncedLuaHandle;
		CUnsyncedLuaHandle unsyncedLuaHandle;

	public:
		static void ClearGameParams() { spring::clear_unordered_map(gameParams); }
		static const LuaRulesParams::Params& GetGameParams() { return gameParams; }

	private:
		//FIXME: add to CREG?
		static LuaRulesParams::Params  gameParams;
		friend class LuaSyncedCtrl;
};


#endif /* LUA_HANDLE_SYNCED */
