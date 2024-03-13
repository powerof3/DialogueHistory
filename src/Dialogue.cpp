#include "Dialogue.h"

#include "GlobalHistory.h"
#include "ImGui/Styles.h"

void TimeStamp::FromYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day)
{
	auto gmst = RE::GameSettingCollection::GetSingleton();

	//standalone Calendar::GetMonthName()
	const auto get_month_name = [&](std::uint32_t a_month) {
		RE::Setting* setting = nullptr;
		switch (a_month) {
		case RE::Calendar::Month::kMorningStar:
			setting = gmst->GetSetting("sMonthJanuary");
			break;
		case RE::Calendar::Month::kSunsDawn:
			setting = gmst->GetSetting("sMonthFebruary");
			break;
		case RE::Calendar::Month::kFirstSeed:
			setting = gmst->GetSetting("sMonthMarch");
			break;
		case RE::Calendar::Month::kRainsHand:
			setting = gmst->GetSetting("sMonthApril");
			break;
		case RE::Calendar::Month::kSecondSeed:
			setting = gmst->GetSetting("sMonthMay");
			break;
		case RE::Calendar::Month::kMidyear:
			setting = gmst->GetSetting("sMonthJune");
			break;
		case RE::Calendar::Month::kSunsHeight:
			setting = gmst->GetSetting("sMonthJuly");
			break;
		case RE::Calendar::Month::kLastSeed:
			setting = gmst->GetSetting("sMonthAugust");
			break;
		case RE::Calendar::Month::kHearthfire:
			setting = gmst->GetSetting("sMonthSeptember");
			break;
		case RE::Calendar::Month::kFrostfall:
			setting = gmst->GetSetting("sMonthOctober");
			break;
		case RE::Calendar::Month::kSunsDusk:
			setting = gmst->GetSetting("sMonthNovember");
			break;
		case RE::Calendar::Month::kEveningStar:
			setting = gmst->GetSetting("sMonthDecember");
			break;
		default:
			setting = nullptr;
			break;
		}
		return setting ? setting->GetString() : "Bad Month";
	};

	//standalone Calendar::GetOrdinalSuffix()
	const auto get_suffix = [&](std::uint32_t a_day) {
		switch (a_day) {
		case 1:
		case 21:
		case 31:
			return gmst->GetSetting("sFirstOrdSuffix")->GetString();
		case 2:
		case 22:
			return gmst->GetSetting("sSecondOrdSuffix")->GetString();
		case 3:
		case 23:
			return gmst->GetSetting("sThirdOrdSuffix")->GetString();
		default:
			return gmst->GetSetting("sDefaultOrdSuffix")->GetString();
		}
	};

	format = std::format("{}{}{}{}, 4E {}", a_day, get_suffix(a_day), gmst->GetSetting("sOf")->GetString(), get_month_name(a_month), a_year);
	time = string::to_num<std::uint32_t>(std::format("{:03}{:02}{:02}", a_year, a_month, a_day));
}

std::string TimeStamp::GetFormattedHourMin(std::uint32_t a_hour, std::uint32_t a_minute, bool a_12HourFormat)
{
	auto gmst = RE::GameSettingCollection::GetSingleton();

	if (a_12HourFormat) {
		std::string AMPM;
		if (a_hour < 12) {
			if (a_hour == 0) {
				a_hour = 12;
			}
			AMPM = gmst->GetSetting("sTimeAM")->GetString();
		} else {
			if (a_hour > 12) {
				a_hour -= 12;
			}
			AMPM = gmst->GetSetting("sTimePM")->GetString();
		}
		return std::format("{:02}:{:02} {}", a_hour, a_minute, AMPM);
	} else {
		return std::format("{:02}:{:02}", a_hour, a_minute);
	}
}

void TimeStamp::FromHourMin(std::uint32_t a_hour, std::uint32_t a_minute, const std::string& a_speaker, bool a_12HourFormat)
{
	format = std::format("{} - {}", GetFormattedHourMin(a_hour, a_minute, a_12HourFormat), a_speaker);
	time = string::to_num<std::uint32_t>(std::format("{:02}{:02}", a_hour, a_minute));
}

void TimeStamp::SwitchHourFormat(bool a_12HourFormat)
{
	std::uint32_t hour = time / 100;
	std::uint32_t minute = time % 100;

	auto hourMin = GetFormattedHourMin(hour, minute, a_12HourFormat);
	format = std::format("{}{}", hourMin, format.substr(format.find(" - ")));
}

Dialogue::Line::Line(RE::TESObjectREFR* a_speaker, const std::string& a_line, const std::string& a_voice) :
	line(a_line),
	voice(a_voice),
	name(a_speaker->GetDisplayFullName()),
	isPlayer(a_speaker->IsPlayerRef()),
	hovered(false)
{
	id.SetNumericID(a_speaker->GetFormID());
}

void Dialogue::GenerateTimeStamp(const std::tm& a_time)
{
	auto dateStr = std::format("{:03}{:02}{:02}", a_time.tm_year, a_time.tm_mon, a_time.tm_mday);
	auto hourMinStr = std::format("{:02}{:02}", a_time.tm_hour, a_time.tm_min);

	date = string::to_num<std::uint32_t>(dateStr);
	hourMin = string::to_num<std::uint32_t>(hourMinStr);
	timeStamp = string::to_num<std::uint64_t>(dateStr.append(hourMinStr));

	logger::info("{}/{}/{}", date, hourMin, timeStamp);
}

void Dialogue::ExtractTimeStamp(std::uint32_t& a_year, std::uint32_t& a_month, std::uint32_t& a_day, std::uint32_t& a_hour, std::uint32_t& a_minute) const
{
	auto timeStampCopy = timeStamp;

	a_minute = timeStampCopy % 100;
	timeStampCopy /= 100;
	a_hour = timeStampCopy % 100;
	timeStampCopy /= 100;
	a_day = timeStampCopy % 100;
	timeStampCopy /= 100;
	a_month = timeStampCopy % 100;
	timeStampCopy /= 100;
	a_year = timeStampCopy;
}

void Dialogue::AddDialogue(RE::TESObjectREFR* a_speaker, const std::string& a_line, const std::string& a_voice)
{
	dialogue.emplace_back(a_speaker, a_line, a_voice);
}

void Dialogue::Draw()
{
	using namespace ImGui;

	bool isGlobalHistoryOpen = MANAGER(GlobalHistory)->IsGlobalHistoryOpen();

	ImGui::Indent();
	for (auto& [id, response, voice, speakerName, isPlayer, hovered] : dialogue) {
		ImGui::TextColoredWrapped(isPlayer ? GetUserStyleColorVec4(USER_STYLE::kPlayerName) : GetUserStyleColorVec4(USER_STYLE::kSpeakerName), std::format("{}: ", speakerName).c_str());

		ImGui::SameLine();

		auto lineColor = isPlayer ? GetUserStyleColorVec4(USER_STYLE::kPlayerLine) : GetUserStyleColorVec4(USER_STYLE::kSpeakerLine);
		lineColor.w = (!isGlobalHistoryOpen || isPlayer || hovered) ? 1.0f : GetUserStyleVar(USER_STYLE::kDisabledTextAlpha);

		ImGui::TextColoredWrapped(lineColor, response.c_str());

		hovered = ImGui::IsItemHovered();

		if (ImGui::IsItemClicked() && isGlobalHistoryOpen) {
			MANAGER(GlobalHistory)->PlayVoiceline(voice);
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
	}
	ImGui::Unindent();
}

void Dialogue::Clear()
{
	timeStamp = 0;
	dialogue.clear();
}
