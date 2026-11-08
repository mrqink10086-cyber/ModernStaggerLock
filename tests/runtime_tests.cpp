#include "RuntimeLogic.h"
#include "StaggerData.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef _WIN32
#	include <string.h>
#else
#	include <strings.h>
#endif

namespace
{
	using ModernStaggerLock::Logic::StaggerEvent;

	void Check(bool a_condition, std::string_view a_message)
	{
		if (!a_condition) {
			throw std::runtime_error(std::string(a_message));
		}
	}

	int LegacyCompare(const char* a_left, const char* a_right)
	{
#ifdef _WIN32
		return _stricmp(a_left, a_right);
#else
		return strcasecmp(a_left, a_right);
#endif
	}

	StaggerEvent LegacyClassify(const std::string& a_event)
	{
		constexpr std::array<std::pair<const char*, StaggerEvent>, 6> names{ { { "staggerStop", StaggerEvent::kStop },
			{ "MSL_SmallStaggerStart", StaggerEvent::kSmall },
			{ "MSL_MediumStaggerStart", StaggerEvent::kMedium },
			{ "MSL_LargeStaggerStart", StaggerEvent::kLarge },
			{ "MSL_LargestStaggerStart", StaggerEvent::kLargest },
			{ "MSL_SpecialStaggerStart", StaggerEvent::kSpecial } } };
		for (const auto& [name, value] : names) {
			if (LegacyCompare(a_event.c_str(), name) == 0) {
				return value;
			}
		}
		return StaggerEvent::kOther;
	}

	std::uint32_t ReferenceHash(std::string_view a_name)
	{
		std::uint32_t hash = 5381u;
		for (char ch : a_name) {
			const auto lower = (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch + ('a' - 'A')) : ch;
			hash = ((hash << 5) + hash) + static_cast<std::uint32_t>(lower);
		}
		return hash;
	}

	std::uint64_t TextHash(std::string_view a_text)
	{
		std::uint64_t hash = 14695981039346656037ull;
		for (unsigned char ch : a_text) {
			hash = (hash ^ ch) * 1099511628211ull;
		}
		return hash;
	}

	std::string Payload(std::string_view a_name, bool a_ignore = false, bool a_includeIgnore = true)
	{
		nlohmann::json json;
		json["SpecialStaggerName"] = std::string(a_name);
		if (a_includeIgnore) {
			json["IgnoreStaggerLevel"] = a_ignore;
		}
		return json.dump();
	}

	void CheckData(const ModernStaggerLock::SpecialStaggerData& a_data, std::uint32_t a_hash, bool a_ignore, std::string_view a_message)
	{
		Check(a_data.GetNameHash() == a_hash, a_message);
		Check(a_data.IsIgnoreStaggerLevel() == a_ignore, a_message);
	}

	void TestHashName()
	{
		const std::array<std::string, 6> names{
			"",
			"MSL_SpecialStaggerStart",
			"Anim_Name_0123456789",
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
			std::string(120000, 'A'),
			"\xC3\xA9_Anim"
		};
		for (const auto& name : names) {
			Check(static_cast<std::uint32_t>(ModernStaggerLock::Logic::HashName(name)) == ReferenceHash(name), "HashName must match uint32 wrapping reference");
		}
		Check(ModernStaggerLock::Logic::HashName("mSl_sPeCiAlStAgGeRsTaRt") == ModernStaggerLock::Logic::HashName("MSL_SpecialStaggerStart"), "HashName must ignore ASCII case");
		std::string alternating;
		alternating.reserve(180000);
		for (std::size_t i = 0; i < 90000; ++i) {
			alternating += i % 2 == 0 ? "Az" : "zA";
		}
		Check(static_cast<std::uint32_t>(ModernStaggerLock::Logic::HashName(alternating)) == ReferenceHash(alternating), "HashName must wrap for long mixed-case input");
	}

	void TestEventClassification()
	{
		constexpr std::array<std::pair<std::string_view, StaggerEvent>, 6> events{ { { "staggerStop", StaggerEvent::kStop },
			{ "MSL_SmallStaggerStart", StaggerEvent::kSmall },
			{ "MSL_MediumStaggerStart", StaggerEvent::kMedium },
			{ "MSL_LargeStaggerStart", StaggerEvent::kLarge },
			{ "MSL_LargestStaggerStart", StaggerEvent::kLargest },
			{ "MSL_SpecialStaggerStart", StaggerEvent::kSpecial } } };
		for (const auto& [name, expected] : events) {
			Check(ModernStaggerLock::Logic::ClassifyEvent(name) == expected, "ClassifyEvent must classify each supported event");
			std::string mixed(name);
			for (std::size_t i = 0; i < mixed.size(); ++i) {
				if (std::isalpha(static_cast<unsigned char>(mixed[i]))) {
					mixed[i] = i % 2 == 0 ? static_cast<char>(std::tolower(static_cast<unsigned char>(mixed[i]))) : static_cast<char>(std::toupper(static_cast<unsigned char>(mixed[i])));
				}
			}
			Check(ModernStaggerLock::Logic::ClassifyEvent(mixed) == expected, "ClassifyEvent must ignore case");
		}
		Check(ModernStaggerLock::Logic::ClassifyEvent("") == StaggerEvent::kOther, "empty event must be unrelated");
		for (const auto& [name, expected] : events) {
			static_cast<void>(expected);
			Check(ModernStaggerLock::Logic::ClassifyEvent(std::string(name.size(), 'x')) == StaggerEvent::kOther, "same-length unknown event must be unrelated");
		}
		Check(ModernStaggerLock::Logic::ClassifyEvent("MSL_MediumStaggerStarx") == StaggerEvent::kOther, "near-match event must be unrelated");

		std::mt19937 random(0xBADC0DEu);
		std::uniform_int_distribution<int> length(0, 48);
		std::uniform_int_distribution<int> letter(0, 51);
		for (int n = 0; n < 12000; ++n) {
			std::string event(static_cast<std::size_t>(length(random)), '\0');
			for (char& ch : event) {
				const auto value = letter(random);
				ch = static_cast<char>(value < 26 ? 'a' + value : 'A' + (value - 26));
			}
			if (n % 2 == 0) {
				event = std::string(events[static_cast<std::size_t>(n) % events.size()].first);
			}
			if (n % 3 == 0) {
				for (std::size_t i = 0; i < event.size(); ++i) {
					event[i] = i % 2 == 0 ? static_cast<char>(std::tolower(static_cast<unsigned char>(event[i]))) : static_cast<char>(std::toupper(static_cast<unsigned char>(event[i])));
				}
			}
			Check(ModernStaggerLock::Logic::ClassifyEvent(event) == LegacyClassify(event), "ClassifyEvent differential mismatch");
		}
	}

	void TestRecoveryEvents()
	{
		ModernStaggerLock::Logic::RecoveryEvents recovery;
		recovery.Assign({ "zeta", "BETA", "alpha", "Alpha", "beta", "Gamma", "ZETA" });
		Check(recovery.Values().size() == 4, "RecoveryEvents must remove case-insensitive duplicates");
		for (std::size_t i = 1; i < recovery.Values().size(); ++i) {
			Check(ModernStaggerLock::Logic::CompareInsensitive(recovery.Values()[i - 1], recovery.Values()[i]) < 0, "RecoveryEvents values must be sorted case-insensitively");
		}
		Check(recovery.Contains("ALPHA") && recovery.Contains("beta") && recovery.Contains("GAMMA") && recovery.Contains("Zeta"), "RecoveryEvents must find mixed-case values");
		Check(!recovery.Contains("") && !recovery.Contains("missing"), "RecoveryEvents must reject absent values");
		recovery.Assign({ "OnlyNew", "onlynew", "Second" });
		Check(recovery.Values().size() == 2, "RecoveryEvents Assign must rebuild and deduplicate values");
		Check(recovery.Contains("ONLYNEW") && recovery.Contains("second"), "RecoveryEvents Assign must expose replacement values");
		Check(!recovery.Contains("alpha") && !recovery.Contains("gamma"), "RecoveryEvents Assign must discard prior values");
		recovery.Assign({});
		Check(recovery.Values().empty() && !recovery.Contains("OnlyNew"), "RecoveryEvents must support empty reload");
	}

	void TestStaggerLevel()
	{
		const std::array<float, 4> thresholds{ 1.0f, 2.0f, 3.0f, 4.0f };
		Check(ModernStaggerLock::Logic::StaggerLevel(0.0f, thresholds) == 1, "magnitude below all thresholds must remain level one");
		Check(ModernStaggerLock::Logic::StaggerLevel(1.0f, thresholds) == 1, "first threshold is inclusive");
		Check(ModernStaggerLock::Logic::StaggerLevel(1.999f, thresholds) == 1, "magnitude below second threshold must remain level one");
		Check(ModernStaggerLock::Logic::StaggerLevel(2.0f, thresholds) == 2, "second threshold is inclusive");
		Check(ModernStaggerLock::Logic::StaggerLevel(3.0f, thresholds) == 3, "third threshold is inclusive");
		Check(ModernStaggerLock::Logic::StaggerLevel(4.0f, thresholds) == 4, "fourth threshold is inclusive");
		Check(ModernStaggerLock::Logic::StaggerLevel(100.0f, thresholds) == 4, "magnitude above all thresholds must use the highest level");
		Check(ModernStaggerLock::Logic::StaggerLevel(std::numeric_limits<float>::quiet_NaN(), thresholds) == 1, "NaN magnitude must preserve the original fallback");
		const std::array<float, 4> unordered{ 4.0f, 1.0f, 3.0f, 2.0f };
		Check(ModernStaggerLock::Logic::StaggerLevel(3.0f, unordered) == 4, "unsorted thresholds must preserve descending-index evaluation");
		Check(ModernStaggerLock::Logic::StaggerLevel(1.5f, unordered) == 2, "unsorted thresholds must return the highest matching index");
	}

	void TestParseStaggerData()
	{
		ModernStaggerLock::SpecialStaggerData data;
		Check(ModernStaggerLock::ParseStaggerData(Payload("Dragon_Turn", true), data), "ParseStaggerData must accept valid JSON");
		CheckData(data, ReferenceHash("Dragon_Turn"), true, "ParseStaggerData must fill name hash and ignore flag");
		Check(ModernStaggerLock::ParseStaggerData(Payload("DefaultFlag", false, false), data), "ParseStaggerData must accept omitted optional flag");
		CheckData(data, ReferenceHash("DefaultFlag"), false, "omitted ignore flag must default to false");
		Check(ModernStaggerLock::ParseStaggerData(Payload("", true), data), "empty special stagger name must parse");
		CheckData(data, 0u, true, "empty name must produce hash zero");

		const std::array<std::string, 6> invalid{
			"{",
			"{}",
			R"({"SpecialStaggerName":17})",
			R"({"SpecialStaggerName":null})",
			R"({"SpecialStaggerName":"Anim","IgnoreStaggerLevel":"yes"})",
			R"({"SpecialStaggerName":"Anim","IgnoreStaggerLevel":1})"
		};
		for (const auto& text : invalid) {
			const auto beforeHash = data.GetNameHash();
			const auto beforeIgnore = data.IsIgnoreStaggerLevel();
			Check(!ModernStaggerLock::ParseStaggerData(text, data), "ParseStaggerData must reject malformed or incorrectly typed fields");
			CheckData(data, beforeHash, beforeIgnore, "invalid ParseStaggerData must preserve output data");
		}
	}

	void TestPayloadCache()
	{
		static_assert(ModernStaggerLock::StaggerPayloadCache::kCapacity == 32);
		static_assert(ModernStaggerLock::StaggerPayloadCache::kMaxKeyLength == 4096);
		ModernStaggerLock::StaggerPayloadCache cache;
		ModernStaggerLock::SpecialStaggerData data;

		const auto valid = Payload("CachedValid", true);
		Check(cache.Find(valid, data), "cache must parse a valid payload");
		CheckData(data, ReferenceHash("CachedValid"), true, "cache valid payload result must match parsed data");
		Check(cache.Find(valid, data), "cache must return a repeated valid payload");
		CheckData(data, ReferenceHash("CachedValid"), true, "repeated cached payload must keep its data");

		data = ModernStaggerLock::SpecialStaggerData{};
		Check(!cache.Find("{invalid", data), "cache must reject invalid payload");
		CheckData(data, 0u, false, "invalid cache lookup must preserve output");
		Check(!cache.Find("{invalid", data), "cache must return a repeated invalid payload as invalid");
		CheckData(data, 0u, false, "repeated invalid lookup must preserve output");

		std::string mutableText = Payload("BeforeMutation", false);
		Check(cache.Find(mutableText, data), "cache must accept an owned-key input");
		mutableText = Payload("AfterMutation", true);
		Check(cache.Find(Payload("BeforeMutation", false), data), "cache key must remain valid after caller string mutation");
		CheckData(data, ReferenceHash("BeforeMutation"), false, "cache must own its key and parsed value");
		Check(cache.Find(mutableText, data), "changed payload text must be parsed as a new value");
		CheckData(data, ReferenceHash("AfterMutation"), true, "changed payload must not return the prior cached value");

		const auto emptyName = Payload("", true);
		Check(cache.Find(emptyName, data), "cache must accept an empty name");
		CheckData(data, 0u, true, "cached empty name must retain the ignore flag");
		const auto defaultFlag = Payload("CachedDefault", false, false);
		Check(cache.Find(defaultFlag, data), "cache must accept omitted optional fields");
		CheckData(data, ReferenceHash("CachedDefault"), false, "cached omitted flag must default to false");
		const auto typedError = R"({"SpecialStaggerName":"Typed","IgnoreStaggerLevel":"true"})";
		Check(!cache.Find(typedError, data), "cache must reject typed field errors");

		std::array<std::string, ModernStaggerLock::StaggerPayloadCache::kCapacity> allSlots;
		std::array<bool, ModernStaggerLock::StaggerPayloadCache::kCapacity> found{};
		for (std::size_t nonce = 0; nonce < 10000; ++nonce) {
			auto candidate = nlohmann::json{ { "SpecialStaggerName", "AllSlots" }, { "Nonce", nonce } }.dump();
			const auto slot = static_cast<std::size_t>(TextHash(candidate) % allSlots.size());
			if (!found[slot]) {
				found[slot] = true;
				allSlots[slot] = std::move(candidate);
				if (std::all_of(found.begin(), found.end(), [](bool value) { return value; })) {
					break;
				}
			}
		}
		Check(std::all_of(found.begin(), found.end(), [](bool value) { return value; }), "test must generate keys for every cache slot");
		ModernStaggerLock::StaggerPayloadCache fullCache;
		for (const auto& text : allSlots) {
			Check(fullCache.Find(text, data), "cache must accept valid keys across all 32 slots");
		}
		for (const auto& text : allSlots) {
			Check(fullCache.Find(text, data), "all 32 distinct cache slots must retain their keys");
			CheckData(data, ReferenceHash("AllSlots"), false, "all-slot cache value must be correct");
		}

		std::array<std::string, 2> colliding;
		std::array<bool, ModernStaggerLock::StaggerPayloadCache::kCapacity> seen{};
		const auto collisionBaseA = Payload("CollisionA", false);
		std::size_t collisionSlot = 0;
		collisionSlot = static_cast<std::size_t>(TextHash(collisionBaseA) % seen.size());
		for (std::size_t nonce = 0; nonce < 10000; ++nonce) {
			auto candidate = nlohmann::json{ { "SpecialStaggerName", "CollisionB" }, { "IgnoreStaggerLevel", true }, { "Nonce", nonce } }.dump();
			if (static_cast<std::size_t>(TextHash(candidate) % seen.size()) == collisionSlot) {
				colliding = { collisionBaseA, std::move(candidate) };
				break;
			}
		}
		Check(!colliding[0].empty() && !colliding[1].empty(), "test must generate colliding cache keys");
		Check(static_cast<std::size_t>(TextHash(colliding[0]) % seen.size()) == collisionSlot, "first collision key must map to expected slot");
		ModernStaggerLock::StaggerPayloadCache collisionCache;
		Check(collisionCache.Find(colliding[0], data), "first colliding payload must parse");
		CheckData(data, ReferenceHash("CollisionA"), false, "first colliding payload data must be correct");
		Check(collisionCache.Find(colliding[1], data), "second colliding payload must replace slot");
		CheckData(data, ReferenceHash("CollisionB"), true, "second colliding payload must return replacement value");
		Check(collisionCache.Find(colliding[0], data), "evicted first colliding payload must be reparsed correctly");
		CheckData(data, ReferenceHash("CollisionA"), false, "reparsed collision value must be correct");

		auto exactlyMax = Payload("AtMax");
		exactlyMax.append(ModernStaggerLock::StaggerPayloadCache::kMaxKeyLength - exactlyMax.size(), ' ');
		Check(exactlyMax.size() == ModernStaggerLock::StaggerPayloadCache::kMaxKeyLength, "boundary payload must be exactly 4096 bytes");
		Check(cache.Find(exactlyMax, data), "cache must accept a payload at the key length limit");
		CheckData(data, ReferenceHash("AtMax"), false, "boundary payload must parse correctly");
		auto overMax = Payload("OverMax");
		overMax.append(ModernStaggerLock::StaggerPayloadCache::kMaxKeyLength + 1 - overMax.size(), ' ');
		Check(overMax.size() == ModernStaggerLock::StaggerPayloadCache::kMaxKeyLength + 1, "long payload must exceed key length limit");
		Check(cache.Find(overMax, data), "payload over 4096 bytes must still parse");
		CheckData(data, ReferenceHash("OverMax"), false, "over-limit payload must parse correctly");

		ModernStaggerLock::StaggerPayloadCache firstCache;
		ModernStaggerLock::StaggerPayloadCache secondCache;
		const auto independent = Payload("Independent", true);
		Check(firstCache.Find(independent, data) && secondCache.Find(independent, data), "independent caches must each accept the same payload");
		const auto independentSlot = static_cast<std::size_t>(TextHash(independent) % ModernStaggerLock::StaggerPayloadCache::kCapacity);
		bool evictedInFirst = false;
		for (std::size_t i = 0; i < 10000 && !evictedInFirst; ++i) {
			const auto text = Payload("CacheLocal" + std::to_string(i), (i % 2) != 0);
			if (static_cast<std::size_t>(TextHash(text) % ModernStaggerLock::StaggerPayloadCache::kCapacity) == independentSlot) {
				Check(firstCache.Find(text, data), "first cache must process a colliding key");
				evictedInFirst = true;
			}
		}
		Check(evictedInFirst, "test must generate an eviction for the first cache");
		Check(secondCache.Find(independent, data), "churning one cache must not affect another cache");
		CheckData(data, ReferenceHash("Independent"), true, "second cache must retain its own result");
	}

	std::string RandomPayload(std::mt19937& a_random, int a_variant)
	{
		std::uniform_int_distribution<int> nameLength(0, 80);
		std::uniform_int_distribution<int> character(0, 51);
		std::string name(static_cast<std::size_t>(nameLength(a_random)), '\0');
		for (char& ch : name) {
			const auto value = character(a_random);
			ch = static_cast<char>(value < 26 ? 'a' + value : 'A' + (value - 26));
		}
		switch (a_variant % 8) {
		case 0:
			return Payload(name, (a_variant & 1) != 0);
		case 1:
			return Payload(name, false, false);
		case 2:
			return R"({"SpecialStaggerName":)";
		case 3:
			return "{}";
		case 4:
			return R"({"SpecialStaggerName":false})";
		case 5:
			return std::string(R"({"SpecialStaggerName":")") + name + R"(","IgnoreStaggerLevel":7})";
		case 6:
			return std::string(R"({"SpecialStaggerName":")") + name + R"(","IgnoreStaggerLevel":"no"})";
		default:
			return std::string("{\"SpecialStaggerName\":\"") + name + "\",\"IgnoreStaggerLevel\":" + ((a_variant & 1) ? "true" : "false") + "}";
		}
	}

	void TestRandomizedDifferential()
	{
		std::mt19937 random(0x51A66E7u);
		ModernStaggerLock::StaggerPayloadCache cache;
		for (int i = 0; i < 6000; ++i) {
			const auto text = RandomPayload(random, i);
			ModernStaggerLock::SpecialStaggerData direct;
			ModernStaggerLock::SpecialStaggerData cached;
			const bool directValid = ModernStaggerLock::ParseStaggerData(text, direct);
			const bool cachedValid = cache.Find(text, cached);
			Check(directValid == cachedValid, "repeated cached parser differential validity mismatch");
			if (directValid) {
				CheckData(cached, direct.GetNameHash(), direct.IsIgnoreStaggerLevel(), "cached parser data must match direct parser data");
			} else {
				CheckData(cached, 0u, false, "invalid cached parse must preserve default output");
			}
		}
	}
}

int main()
{
	try {
		TestHashName();
		TestEventClassification();
		TestRecoveryEvents();
		TestStaggerLevel();
		TestParseStaggerData();
		TestPayloadCache();
		TestRandomizedDifferential();
		std::cout << "runtime tests passed\n";
		return 0;
	} catch (const std::exception& error) {
		std::cerr << "runtime tests failed: " << error.what() << '\n';
		return 1;
	}
}
