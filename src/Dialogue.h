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

	void FromYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day);
	void FromHourMin(std::uint32_t a_hour, std::uint32_t a_minute, const std::string& a_speaker, bool a_12HourFormat);
	void SwitchHourFormat(bool a_12HourFormat);

	// members
	std::uint32_t time{};
	std::string   format{};

private:
	std::string GetFormattedHourMin(std::uint32_t a_hour, std::uint32_t a_minute, bool a_12HourFormat);
};

struct Dialogue
{
	struct Line
	{
		Line() = default;
		Line(RE::TESObjectREFR* a_speaker, const std::string& a_line, const std::string& a_voice);

		// members
		RE::BGSNumericIDIndex id;
		std::string           line;
		std::string           voice;

		// skip write
		std::string name{};
		bool        isPlayer{};
		bool        hovered{};

		struct glaze
		{
			using T = Line;
			static constexpr auto value = glz::object(
				"id", &T::id,
				"line", &T::line,
				"wav", &T::voice,
				"name", glz::hide(&T::name),
				"hovered", glz::hide(&T::hovered),
				"isPC", glz::hide(&T::isPlayer));
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

	void GenerateTimeStamp(const std::tm& a_time);
	void ExtractTimeStamp(std::uint32_t& a_year, std::uint32_t& a_month, std::uint32_t& a_day, std::uint32_t& a_hour, std::uint32_t& a_minute) const;

	void AddDialogue(RE::TESObjectREFR* a_speaker, const std::string& a_line, const std::string& a_voice);
	void Draw();
	void Clear();

	// members
	std::uint64_t     timeStamp;
	std::uint32_t     date;
	std::uint32_t     hourMin;
	std::vector<Line> dialogue{};

	struct glaze
	{
		using T = Dialogue;
		static constexpr auto value = glz::object(
			"time", &T::timeStamp,
			"lines", &T::dialogue);
	};
};
