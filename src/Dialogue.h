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

	bool operator<(const TimeStamp& a_rhs) const
	{
		return time < a_rhs.time;
	}
	bool operator>(const TimeStamp& a_rhs) const
	{
		return time > a_rhs.time;
	}
	bool operator==(const TimeStamp& a_rhs) const
	{
		return time == a_rhs.time;
	}

	static std::uint64_t GenerateTimeStamp(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day, std::uint32_t a_hour, std::uint32_t a_minute);
	static std::string   GetFormattedYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day);
	static std::string   GetFormattedHourMin(std::uint32_t a_hour, std::uint32_t a_minute, bool a_12HourFormat);

	void FromYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day);
	void FromHourMin(std::uint32_t a_hour, std::uint32_t a_minute, const std::string& a_speaker, bool a_12HourFormat);
	void SwitchHourFormat(bool a_12HourFormat);

	// members
	std::uint64_t time{};
	std::string   format{};
};

struct Dialogue
{
	struct Line
	{
		Line() = default;
		Line(RE::TESObjectREFR* a_speaker, const std::string& a_line, const std::string& a_voice);

		// members
		std::string line;
		std::string voice;

		// skip write
		std::string name{};
		bool        isPlayer{};
		bool        hovered{};

		struct glaze
		{
			using T = Line;
			static constexpr auto value = glz::object(
				"line", &T::line,
				"wav", &T::voice,
				"name", glz::hide(&T::name),
				"pc", glz::hide(&T::isPlayer),
				"hovered", glz::hide(&T::hovered));
		};
	};

	Dialogue() = default;

	bool operator==(const Dialogue& a_rhs) const
	{
		return timeStamp == a_rhs.timeStamp;
	}
	bool operator<(const Dialogue& a_rhs) const
	{
		return timeStamp < a_rhs.timeStamp;
	}
	bool operator>(const Dialogue& a_rhs) const
	{
		return timeStamp > a_rhs.timeStamp;
	}
	bool empty() const
	{
		return dialogue.empty();
	}

	void Initialize(RE::TESObjectREFR* a_speaker);
	void Initialize(const std::tm& a_time);

	void        ExtractTimeStamp(std::uint32_t& a_year, std::uint32_t& a_month, std::uint32_t& a_day, std::uint32_t& a_hour, std::uint32_t& a_minute) const;
	std::string TimeStampToString(bool a_use12HourFormat) const;

	void AddDialogue(RE::TESObjectREFR* a_speaker, const std::string& a_line, const std::string& a_voice);
	void Draw();
	void Clear();

	// members
	std::uint64_t         timeStamp;
	RE::BGSNumericIDIndex id;
	RE::BGSNumericIDIndex loc;
	std::vector<Line>     dialogue{};

	std::string locName;
	std::string timeAndLoc;
	std::string speakerName;

	struct glaze
	{
		using T = Dialogue;
		static constexpr auto value = glz::object(
			"time", &T::timeStamp,
			"id", &T::id,
			"loc", &T::loc,
			"lines", &T::dialogue,
			"locName", glz::hide(&T::locName),
			"timeLoc", glz::hide(&T::timeAndLoc),
			"speakerName", glz::hide(&T::speakerName));
	};
};
