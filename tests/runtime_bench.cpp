#include "RuntimeLogic.h"
#include "StaggerData.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#	include <string.h>
#else
#	include <strings.h>
#endif

#if defined(_MSC_VER)
#	define RUNTIME_BENCH_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#	define RUNTIME_BENCH_NOINLINE __attribute__((noinline))
#else
#	define RUNTIME_BENCH_NOINLINE
#endif

namespace
{
	using Clock = std::chrono::steady_clock;

	std::uint64_t g_checksum = 0;

	int LegacyCompare(const char* a_left, const char* a_right)
	{
#ifdef _WIN32
		return _stricmp(a_left, a_right);
#else
		return strcasecmp(a_left, a_right);
#endif
	}

	ModernStaggerLock::Logic::StaggerEvent LegacyClassify(const std::string& a_event)
	{
		constexpr std::array<std::pair<const char*, ModernStaggerLock::Logic::StaggerEvent>, 6> names{ { { "staggerStop", ModernStaggerLock::Logic::StaggerEvent::kStop },
			{ "MSL_SmallStaggerStart", ModernStaggerLock::Logic::StaggerEvent::kSmall },
			{ "MSL_MediumStaggerStart", ModernStaggerLock::Logic::StaggerEvent::kMedium },
			{ "MSL_LargeStaggerStart", ModernStaggerLock::Logic::StaggerEvent::kLarge },
			{ "MSL_LargestStaggerStart", ModernStaggerLock::Logic::StaggerEvent::kLargest },
			{ "MSL_SpecialStaggerStart", ModernStaggerLock::Logic::StaggerEvent::kSpecial } } };
		for (const auto& [name, value] : names) {
			if (LegacyCompare(a_event.c_str(), name) == 0) {
				return value;
			}
		}
		return ModernStaggerLock::Logic::StaggerEvent::kOther;
	}

	std::uint32_t EventValue(ModernStaggerLock::Logic::StaggerEvent a_event)
	{
		return static_cast<std::uint32_t>(static_cast<int>(a_event) + 2);
	}

	void RandomizeCase(std::string& a_text, std::mt19937& a_random)
	{
		std::uniform_int_distribution<int> coin(0, 1);
		for (char& ch : a_text) {
			if (ch >= 'a' && ch <= 'z' && coin(a_random)) {
				ch = static_cast<char>(ch - ('a' - 'A'));
			} else if (ch >= 'A' && ch <= 'Z' && coin(a_random)) {
				ch = static_cast<char>(ch + ('a' - 'A'));
			}
		}
	}

	std::vector<std::string> MakeUnrelatedEvents(std::size_t a_count, std::mt19937& a_random)
	{
		std::uniform_int_distribution<int> length(8, 36);
		std::uniform_int_distribution<int> letter(0, 51);
		std::vector<std::string> result;
		result.reserve(a_count);
		for (std::size_t i = 0; i < a_count; ++i) {
			std::string event = "OtherEvent_";
			const auto count = static_cast<std::size_t>(length(a_random));
			for (std::size_t j = 0; j < count; ++j) {
				const auto value = letter(a_random);
				event.push_back(static_cast<char>(value < 26 ? 'a' + value : 'A' + (value - 26)));
			}
			result.push_back(std::move(event));
		}
		return result;
	}

	std::vector<std::string> MakeSupportedEvents(std::size_t a_count, std::mt19937& a_random)
	{
		constexpr std::array<std::string_view, 6> names{
			"staggerStop",
			"MSL_SmallStaggerStart",
			"MSL_MediumStaggerStart",
			"MSL_LargeStaggerStart",
			"MSL_LargestStaggerStart",
			"MSL_SpecialStaggerStart"
		};
		std::uniform_int_distribution<std::size_t> choose(0, names.size() - 1);
		std::vector<std::string> result;
		result.reserve(a_count);
		for (std::size_t i = 0; i < a_count; ++i) {
			std::string event(names[choose(a_random)]);
			RandomizeCase(event, a_random);
			result.push_back(std::move(event));
		}
		return result;
	}

	std::vector<std::string> MakeMixedEvents(std::size_t a_count, std::mt19937& a_random)
	{
		auto supported = MakeSupportedEvents(a_count / 2, a_random);
		auto unrelated = MakeUnrelatedEvents(a_count - supported.size(), a_random);
		supported.insert(supported.end(), std::make_move_iterator(unrelated.begin()), std::make_move_iterator(unrelated.end()));
		std::shuffle(supported.begin(), supported.end(), a_random);
		return supported;
	}

	template <typename Fn>
	double MeasureMedianNsPerOp(Fn&& a_run, std::size_t a_operations)
	{
		for (int warmup = 0; warmup < 2; ++warmup) {
			g_checksum = g_checksum * 1099511628211ull + a_run() + static_cast<std::uint64_t>(warmup);
		}
		std::array<double, 7> samples{};
		for (auto& sample : samples) {
			const auto start = Clock::now();
			const auto checksum = a_run();
			const auto end = Clock::now();
			g_checksum = g_checksum * 1099511628211ull + checksum;
			sample = std::chrono::duration<double, std::nano>(end - start).count() / static_cast<double>(a_operations);
		}
		std::sort(samples.begin(), samples.end());
		return samples[samples.size() / 2];
	}

	template <typename Fn>
	std::uint64_t BenchmarkEvents(const std::vector<std::string>& a_events, std::size_t a_passes, Fn&& a_classify)
	{
		std::uint64_t checksum = 0;
		for (std::size_t pass = 0; pass < a_passes; ++pass) {
			for (const auto& event : a_events) {
				checksum += EventValue(a_classify(event));
			}
		}
		return checksum;
	}

	template <typename Fn>
	std::uint64_t BenchmarkPayloads(const std::vector<std::string>& a_payloads, std::size_t a_passes, Fn&& a_parse)
	{
		std::uint64_t checksum = 0;
		for (std::size_t pass = 0; pass < a_passes; ++pass) {
			for (const auto& payload : a_payloads) {
				ModernStaggerLock::SpecialStaggerData data;
				checksum += a_parse(payload, data) ? data.GetNameHash() + (data.IsIgnoreStaggerLevel() ? 1u : 0u) : 1u;
			}
		}
		return checksum;
	}

	struct LegacyTransition
	{
		std::string animName;
		std::set<std::int32_t> transAnimList;
	};

	RUNTIME_BENCH_NOINLINE bool LegacyCopiedSetContains(const LegacyTransition& a_source, std::int32_t a_query)
	{
		auto copied = a_source;
		return copied.transAnimList.find(a_query) != copied.transAnimList.end();
	}

	std::uint64_t BenchmarkLegacySet(const LegacyTransition& a_set, const std::vector<std::int32_t>& a_queries, std::size_t a_passes)
	{
		std::uint64_t checksum = 0;
		for (std::size_t pass = 0; pass < a_passes; ++pass) {
			for (const auto& query : a_queries) {
				checksum += LegacyCopiedSetContains(a_set, query) ? 1u : 0u;
			}
		}
		return checksum;
	}

	std::uint64_t BenchmarkTransitionVector(const std::vector<std::int32_t>& a_events, const std::vector<std::int32_t>& a_queries, std::size_t a_passes)
	{
		std::uint64_t checksum = 0;
		for (std::size_t pass = 0; pass < a_passes; ++pass) {
			for (const auto& query : a_queries) {
				checksum += std::binary_search(a_events.begin(), a_events.end(), query) ? 1u : 0u;
			}
		}
		return checksum;
	}

	std::uint64_t CacheSlot(std::string_view a_text)
	{
		std::uint64_t hash = 14695981039346656037ull;
		for (unsigned char ch : a_text) {
			hash = (hash ^ ch) * 1099511628211ull;
		}
		return hash % ModernStaggerLock::StaggerPayloadCache::kCapacity;
	}

	std::vector<std::string> MakeDistinctPayloads(std::mt19937& a_random)
	{
		std::uniform_int_distribution<std::uint32_t> value;
		std::vector<std::string> result;
		std::array<bool, 32> slots{};
		for (std::size_t i = 0; i < 8; ++i) {
			nlohmann::json json;
			json["SpecialStaggerName"] = "RuntimeAnim_" + std::to_string(value(a_random));
			json["IgnoreStaggerLevel"] = (value(a_random) & 1u) != 0;
			for (std::size_t nonce = 0; nonce < 1000; ++nonce) {
				json["Nonce"] = nonce;
				auto candidate = json.dump();
				const auto slot = static_cast<std::size_t>(CacheSlot(candidate));
				if (!slots[slot]) {
					slots[slot] = true;
					result.push_back(std::move(candidate));
					break;
				}
			}
		}
		return result;
	}

	void PrintPair(std::string_view a_label, double a_old, double a_new)
	{
		std::cout << std::left << std::setw(30) << a_label
				  << " old " << std::right << std::fixed << std::setprecision(2) << a_old << " ns/op"
				  << "  new " << a_new << " ns/op"
				  << "  old/new " << (a_new == 0.0 ? 0.0 : a_old / a_new) << "x\n";
	}
}

int main(int a_argc, char** a_argv)
{
	std::random_device seedSource;
	const auto seed = a_argc > 1 ? static_cast<std::uint32_t>(std::stoul(a_argv[1])) : static_cast<std::uint32_t>(seedSource()) ^ static_cast<std::uint32_t>(Clock::now().time_since_epoch().count());
	std::mt19937 random(seed);
	std::cout << "Runtime microbench seed " << seed << '\n';

	const auto supported = MakeSupportedEvents(512, random);
	const auto unrelated = MakeUnrelatedEvents(512, random);
	const auto mixed = MakeMixedEvents(512, random);
	constexpr std::size_t eventPasses = 2500;
	const auto eventOperations = eventPasses * supported.size();
	const auto legacySupported = MeasureMedianNsPerOp([&] { return BenchmarkEvents(supported, eventPasses, LegacyClassify); }, eventOperations);
	const auto newSupported = MeasureMedianNsPerOp([&] { return BenchmarkEvents(supported, eventPasses, ModernStaggerLock::Logic::ClassifyEvent); }, eventOperations);
	PrintPair("classifier supported events", legacySupported, newSupported);
	const auto legacyUnrelated = MeasureMedianNsPerOp([&] { return BenchmarkEvents(unrelated, eventPasses, LegacyClassify); }, eventOperations);
	const auto newUnrelated = MeasureMedianNsPerOp([&] { return BenchmarkEvents(unrelated, eventPasses, ModernStaggerLock::Logic::ClassifyEvent); }, eventOperations);
	PrintPair("classifier unrelated events", legacyUnrelated, newUnrelated);
	const auto legacyMixed = MeasureMedianNsPerOp([&] { return BenchmarkEvents(mixed, eventPasses, LegacyClassify); }, eventOperations);
	const auto newMixed = MeasureMedianNsPerOp([&] { return BenchmarkEvents(mixed, eventPasses, ModernStaggerLock::Logic::ClassifyEvent); }, eventOperations);
	PrintPair("classifier mixed events", legacyMixed, newMixed);

	const auto payloads = MakeDistinctPayloads(random);
	if (payloads.size() != 8) {
		std::cerr << "Could not construct eight distinct cache slots\n";
		return 2;
	}
	ModernStaggerLock::StaggerPayloadCache payloadCache;
	ModernStaggerLock::SpecialStaggerData payloadData;
	for (const auto& payload : payloads) {
		if (!payloadCache.Find(payload, payloadData)) {
			std::cerr << "Generated invalid JSON payload\n";
			return 2;
		}
	}
	constexpr std::size_t payloadPasses = 25000;
	const auto payloadOperations = payloadPasses * payloads.size();
	const auto rawParse = MeasureMedianNsPerOp([&] {
		return BenchmarkPayloads(payloads, payloadPasses, [](std::string_view text, ModernStaggerLock::SpecialStaggerData& data) {
			return ModernStaggerLock::ParseStaggerData(text, data);
		});
	},
		payloadOperations);
	const auto cachedParse = MeasureMedianNsPerOp([&] {
		return BenchmarkPayloads(payloads, payloadPasses, [&](std::string_view text, ModernStaggerLock::SpecialStaggerData& data) {
			return payloadCache.Find(text, data);
		});
	},
		payloadOperations);
	PrintPair("JSON parse vs cache hits", rawParse, cachedParse);

	std::vector<std::int32_t> transitionIDs;
	transitionIDs.reserve(32);
	for (std::size_t i = 0; i < 32; ++i) {
		transitionIDs.push_back(static_cast<std::int32_t>(i * 71));
	}
	LegacyTransition legacySet{ "MSL_RuntimeBenchmarkAnimation", { transitionIDs.begin(), transitionIDs.end() } };
	std::vector<std::int32_t> queries;
	queries.reserve(256);
	std::uniform_int_distribution<std::size_t> chooseID(0, transitionIDs.size() - 1);
	for (std::size_t i = 0; i < 256; ++i) {
		queries.push_back(transitionIDs[chooseID(random)] + static_cast<std::int32_t>(i % 2));
	}
	constexpr std::size_t lookupPasses = 250;
	const auto lookupOperations = lookupPasses * queries.size();
	const auto copiedSet = MeasureMedianNsPerOp([&] { return BenchmarkLegacySet(legacySet, queries, lookupPasses); }, lookupOperations);
	const auto constVector = MeasureMedianNsPerOp([&] { return BenchmarkTransitionVector(transitionIDs, queries, lookupPasses); }, lookupOperations);
	PrintPair("transition copy vs const vector", copiedSet, constVector);

	std::vector<std::string> churnPayloads;
	for (std::size_t i = 0; i < 128; ++i) {
		churnPayloads.push_back(nlohmann::json{ { "SpecialStaggerName", "Anim_" + std::to_string(i) } }.dump());
	}
	ModernStaggerLock::StaggerPayloadCache churnCache;
	constexpr std::size_t churnPasses = 1000;
	const auto churnOperations = churnPasses * churnPayloads.size();
	const auto churnRaw = MeasureMedianNsPerOp([&] {
		return BenchmarkPayloads(churnPayloads, churnPasses, ModernStaggerLock::ParseStaggerData);
	},
		churnOperations);
	const auto churnCached = MeasureMedianNsPerOp([&] {
		return BenchmarkPayloads(churnPayloads, churnPasses, [&](std::string_view text, ModernStaggerLock::SpecialStaggerData& data) {
			return churnCache.Find(text, data);
		});
	},
		churnOperations);
	PrintPair("JSON churn 128 keys / 32 slots", churnRaw, churnCached);

	for (const auto count : { 4, 32, 256 }) {
		std::vector<std::string> recoveryNames;
		for (int i = 0; i < count; ++i) recoveryNames.push_back("QuickRecoveryEvent_" + std::to_string(i));
		ModernStaggerLock::Logic::RecoveryEvents recovery;
		recovery.Assign(recoveryNames);
		auto eventQueries = MakeUnrelatedEvents(128, random);
		for (int i = 0; i < 128; ++i) {
			auto query = recoveryNames[static_cast<std::size_t>(random() % count)];
			RandomizeCase(query, random);
			eventQueries.push_back(std::move(query));
		}
		std::shuffle(eventQueries.begin(), eventQueries.end(), random);
		const auto runRecovery = [&](bool a_legacy) {
			std::uint64_t checksum = 0;
			for (int pass = 0; pass < 1000; ++pass) {
				for (const auto& query : eventQueries) {
					checksum += a_legacy ? std::any_of(recoveryNames.begin(), recoveryNames.end(), [&](const auto& name) {
						return LegacyCompare(name.c_str(), query.c_str()) == 0;
					}) :
					                       recovery.Contains(query);
				}
			}
			return checksum;
		};
		const auto oldRecovery = MeasureMedianNsPerOp([&] { return runRecovery(true); }, 1000 * eventQueries.size());
		const auto newRecovery = MeasureMedianNsPerOp([&] { return runRecovery(false); }, 1000 * eventQueries.size());
		PrintPair("recovery events " + std::to_string(count), oldRecovery, newRecovery);
	}

	std::cout << "checksums " << g_checksum << '\n';
	return 0;
}
