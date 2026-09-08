#pragma once

#include "ImGui/IconsFonts.h"
#include "ImGui/Util.h"

template <>
struct glz::meta<RE::BGSNumericIDIndex>
{
	using T = RE::BGSNumericIDIndex;
	static constexpr auto value = array(&T::data1, &T::data2, &T::data3);
};

// unique id + formatted time string (ie. year+day+month)
struct TimeStamp
{
	TimeStamp() = default;
	TimeStamp(std::uint64_t a_timeStamp, const std::string& a_format);

	bool operator==(const TimeStamp& a_rhs) const
	{
		return time == a_rhs.time;
	}
	auto operator<=>(const TimeStamp& a_rhs) const
	{
		return time <=> a_rhs.time;
	}

	static const char* GetMonthName(std::uint32_t a_month);
	static const char* GetOrdinalSuffix(std::uint32_t a_day);

	static std::uint64_t GenerateTimeStamp(std::tm a_time);
	static std::tm       ExtractTimeStamp(std::uint64_t a_timeStamp);

	static std::string GetFormattedYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day);
	static std::string GetFormattedHourMin(std::uint32_t a_hour, std::uint32_t a_minute, bool a_12HourFormat);

	void FromYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day);
	void FromHourMin(std::uint32_t a_hour, std::uint32_t a_minute, const std::string& a_speaker, bool a_12HourFormat);
	void SwitchHourFormat(bool a_12HourFormat);

	// members
	std::uint64_t time{};
	std::string   format{};
};

// base class for Dialogue and Monologue
struct Speech
{
	struct Line
	{
		Line() = default;
		Line(std::string a_line, std::string a_voice);

		void Sanitize();

		// members
		std::string line;
		std::string voice;

		struct glaze
		{
			using T = Line;
			static constexpr auto value = glz::object(
				"line", &T::line,
				"wav", &T::voice);
		};
	};

	Speech() = default;
	Speech(const std::tm& a_time, RE::TESObjectREFR* a_speaker);

	bool operator==(const Speech& a_rhs) const
	{
		return timeStamp == a_rhs.timeStamp;
	}
	auto operator<=>(const Speech& a_rhs) const
	{
		return timeStamp <=> a_rhs.timeStamp;
	}

	void    Initialize(RE::TESObjectREFR* a_speaker);
	void    Initialize(const std::tm& a_time);
	std::tm ExtractTimeStamp() const;

	bool ResolveIDs();

	void Clear();

	// members
	std::uint64_t              timeStamp;
	RE::BGSNumericIDIndex      id;
	RE::BGSNumericIDIndex      loc;
	std::string                locName;
	std::string                speakerName;
	std::optional<std::string> tempSpeaker{};
};

// Conversations between PC + NPC
struct Dialogue : public Speech
{
	struct Line : public Speech::Line
	{
		Line() = default;
		Line(RE::TESObjectREFR* a_speaker, std::string a_line, std::string a_voice);

		// members
		bool isPlayer{};  // skip write

		struct glaze
		{
			using T = Line;
			static constexpr auto value = glz::object(
				"line", &T::line,
				"wav", &T::voice,
				"pc", glz::hide(&T::isPlayer));
		};
	};

	Dialogue() = default;
	Dialogue(const std::tm& a_time, RE::TESObjectREFR* a_speaker);

	bool empty() const
	{
		return dialogue.empty();
	}

	void        AddDialogue(RE::TESObjectREFR* a_speaker, std::string a_line, std::string a_voice);
	std::string TimeStampToString(bool a_use12HourFormat) const;

	void Draw();
	void Clear();
	void RefreshContents();

	bool Resolve();

	// members
	std::vector<Dialogue::Line> dialogue{};
	std::string                 timeAndLoc{};
	float                       nameWidth{ 0.0f };
	float                       colonWidth{ 0.0f };
	bool                        refreshContents{ true };

	struct glaze
	{
		using T = Dialogue;
		static constexpr auto value = glz::object(
			"time", &T::timeStamp,
			"id", &T::id,
			"speaker", &T::tempSpeaker,
			"loc", &T::loc,
			"lines", &T::dialogue);
	};
};

// NPC
struct Monologue : public Speech
{
	Monologue() = default;
	Monologue(std::tm& a_time, RE::TESObjectREFR* a_speaker, std::string a_line, std::string a_voice, RE::TESTopic* a_topic);

	bool Resolve();

	// members
	Speech::Line          line{};
	RE::BGSNumericIDIndex topic;
	std::int32_t          dialogueType{ -1 };
	std::string           hourMinTimeStamp;

	struct glaze
	{
		using T = Monologue;
		static constexpr auto value = glz::object(
			"time", &T::timeStamp,
			"id", &T::id,
			"speaker", &T::tempSpeaker,
			"loc", &T::loc,
			"line", &T::line,
			"topic", &T::topic);
	};
};

struct Monologues
{
	bool operator==(const Monologues& a_rhs) const
	{
		return monologues == a_rhs.monologues;
	}

	bool empty() const;
	void clear();

	void Draw();
	void RefreshContents();

	// members
	std::vector<Monologue*> monologues{};
	bool                    refreshContents{ true };
	float                   timeWidth{ 0.0f };
	float                   nameWidth{ 0.0f };
	float                   colonWidth{ 0.0f };
};
