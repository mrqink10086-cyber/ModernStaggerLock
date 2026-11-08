#include "Hooks.h"
#include "Settings.h"
#include "SpecialStaggerHandler.h"

namespace ModernStaggerLock
{
	bool NotifyAnimationGraphHook::ProcessNotification(RE::IAnimationGraphManagerHolder* a_graphMgr, const RE::BSFixedString& a_eventName)
	{
		if (!MSLSettings::IsReady()) {
			return false;
		}
		const std::string_view eventName = a_eventName.c_str();
		const bool staggerStart = Logic::EqualInsensitive(eventName, "StaggerStart");
		if (!staggerStart && !MSLSettings::quickRecoveryEvents.Contains(eventName)) {
			return false;
		}

		auto actorRef = a_graphMgr ? skyrim_cast<RE::Actor*>(a_graphMgr) : nullptr;
		if (!actorRef) {
			return false;
		}

		const auto& names = AnimationNames::Get();
		if (staggerStart) {
			// Set Stagger Direction
			float staggerDirection = 0.f;
			actorRef->GetGraphVariableFloat(names.staggerDirection, staggerDirection);
			actorRef->SetGraphVariableInt(names.mslDirection, staggerDirection < 0.25f || staggerDirection > 0.75f ? 0 : 1);

			//Set Stagger Level
			float staggerMagnitude = 0.f;
			actorRef->GetGraphVariableFloat(names.staggerMagnitude, staggerMagnitude);
			const auto staggerLevel = MSLSettings::GetSingleton()->GetStaggerLevel(staggerMagnitude);
			actorRef->SetGraphVariableInt(names.mslLevel, staggerLevel);

			//Set Speical Stagger Type
			SpecialStaggerHandler::GetSingleton()->ProcessSpeicalStagger(actorRef, staggerLevel);
			return false;
		}

		if (!actorRef->IsStaggering()) {
			return false;
		}
		bool recovery = false;
		return actorRef->GetGraphVariableBool(names.recovery, recovery) && recovery;
	}

	bool DisableStaggerJumpHook::Hook_CanProcess(RE::JumpHandler* a_handler, RE::InputEvent* a_event)
	{
		const auto result = func(a_handler, a_event);
		if (result && MSLSettings::IsReady() && MSLSettings::GetSingleton()->IsJumpDisabled()) {
			const auto playerRef = RE::PlayerCharacter::GetSingleton();
			return !playerRef || !playerRef->IsStaggering();
		}
		return result;
	}

	EventResult AnimEventHook::ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink, const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
	{
		if (MSLSettings::IsReady() && a_event && a_event->holder) {
			const auto kind = Logic::ClassifyEvent(a_event->tag.c_str());
			if (kind != Logic::StaggerEvent::kOther) {
				if (auto actor = a_event->holder->As<RE::Actor>()) {
					const auto& names = AnimationNames::Get();
					if (kind != Logic::StaggerEvent::kSpecial) {
						actor->SetGraphVariableInt(names.specialID, 0);
					}
					const auto counterIndex = static_cast<std::int32_t>(kind);
					std::int32_t count = 0;
					if (counterIndex >= 0) {
						actor->GetGraphVariableInt(names.counters[counterIndex], count);
						count = Logic::NextCounter(count);
					}
					for (std::int32_t i = 0; i < static_cast<std::int32_t>(names.counters.size()); ++i) {
						actor->SetGraphVariableInt(names.counters[i], i == counterIndex ? count : 0);
					}
				}
			}
		}
		return _ProcessEvent(a_sink, a_event, a_eventSource);
	}

	void ActorUpdateHook::CharacterEx::Hook_Update(float a_delta)
	{
		func(this, a_delta);
		if (!MSLSettings::IsReady()) {
			return;
		}
		if (const auto controller = GetCharController()) {
			SetGraphVariableInt(AnimationNames::Get().characterState, static_cast<std::int32_t>(controller->context.currentState));
		}
	}

	void ActorUpdateHook::PlayerEx::Hook_Update(float a_delta)
	{
		func(this, a_delta);
		if (!MSLSettings::IsReady()) {
			return;
		}
		if (const auto controller = GetCharController()) {
			SetGraphVariableInt(AnimationNames::Get().characterState, static_cast<std::int32_t>(controller->context.currentState));
		}
	}

	namespace
	{
		void UpdateLandEvent(RE::TESActionData* a_data)
		{
			if (MSLSettings::IsReady() && a_data && a_data->source && a_data->action && Logic::EqualInsensitive(a_data->action->GetFormEditorID(), "ActionLand")) {
				const auto& names = AnimationNames::Get();
				bool specialFalling = false;
				if (a_data->source->GetGraphVariableBool(names.specialFalling, specialFalling) && specialFalling) {
					a_data->animEvent = names.specialLandStart;
				}
			}
		}
	}

	bool PerformLandActionHook_NPC::PerformLandAction(RE::TESActionData* a_data)
	{
		UpdateLandEvent(a_data);
		return _PerformAction(a_data);
	}

	bool PerformLandActionHook_PC::PerformLandAction(RE::TESActionData* a_data)
	{
		UpdateLandEvent(a_data);
		return _PerformAction(a_data);
	}
}
