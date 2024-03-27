#include "Dialogue.h"

#include "GlobalHistory.h"
#include "ImGui/Styles.h"

TimeStamp::TimeStamp(std::uint64_t a_timeStamp, const std::string& a_format) :
	time(a_timeStamp),
	format(std::format("{}##{}", a_format, a_timeStamp))
{}

std::uint64_t TimeStamp::GenerateTimeStamp(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day, std::uint32_t a_hour, std::uint32_t a_minute)
{
	return string::to_num<std::uint64_t>(std::format("{:03}{:02}{:02}{:02}{:02}", a_year, a_month, a_day, a_hour, a_minute));
}

std::string TimeStamp::GetFormattedYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day)
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

	return std::format("{}{}{}{}, 4E {}", a_day, get_suffix(a_day), gmst->GetSetting("sOf")->GetString(), get_month_name(a_month), a_year);
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

void TimeStamp::FromYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day)
{
	time = string::to_num<std::uint32_t>(std::format("{:03}{:02}{:02}", a_year, a_month, a_day));
	format = std::format("{}##{}", GetFormattedYearMonthDay(a_year, a_month, a_day), time);
}

void TimeStamp::FromHourMin(std::uint32_t a_hour, std::uint32_t a_minute, const std::string& a_speaker, bool a_12HourFormat)
{
	time = string::to_num<std::uint32_t>(std::format("{:02}{:02}", a_hour, a_minute));
	format = std::format("{} - {}##{}", GetFormattedHourMin(a_hour, a_minute, a_12HourFormat), a_speaker, time);
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
{}

void Dialogue::Initialize(RE::TESObjectREFR* a_speaker)
{
	if (!speakerName.empty()) {
		return;
	}

	if (a_speaker) {
		id.SetNumericID(a_speaker->GetFormID());
		RE::TESForm* cellOrLoc = a_speaker->GetCurrentLocation();
		if (!cellOrLoc) {
			cellOrLoc = a_speaker->GetParentCell();
		}
		if (cellOrLoc) {
			loc.SetNumericID(cellOrLoc->GetFormID());
			locName = cellOrLoc->GetName();
			if (locName.empty()) {
				locName = "$DH_UnknownLocation"_T;
			}
		}
		speakerName = a_speaker->GetDisplayFullName();
	}
}

void Dialogue::Initialize(const std::tm& a_time)
{
	timeStamp = TimeStamp::GenerateTimeStamp(a_time.tm_year, a_time.tm_mon, a_time.tm_mday, a_time.tm_hour, a_time.tm_min);
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

std::string Dialogue::TimeStampToString(bool a_use12HourFormat) const
{
	std::uint32_t year, month, day, hour, min;
	ExtractTimeStamp(year, month, day, hour, min);

	return std::format("{} - {}", TimeStamp::GetFormattedYearMonthDay(year, month, day), TimeStamp::GetFormattedHourMin(hour, min, a_use12HourFormat));
}

void Dialogue::AddDialogue(RE::TESObjectREFR* a_speaker, const std::string& a_line, const std::string& a_voice)
{
	Initialize(a_speaker);
	dialogue.emplace_back(a_speaker, a_line, a_voice);
}

void Dialogue::Draw()
{
	using namespace ImGui;

	bool isGlobalHistoryOpen = MANAGER(GlobalHistory)->IsGlobalHistoryOpen();

	ImGui::Indent();
	{
		if (isGlobalHistoryOpen) {
			ImGui::PushFont(MANAGER(IconFont)->GetHeaderFont());
			{
				ImGui::CenteredText(speakerName.c_str(), false);
			}
			ImGui::PopFont();

			if (timeAndLoc.empty()) {
				timeAndLoc = std::format("{} - {}", TimeStampToString(MANAGER(GlobalHistory)->Use12HourFormat()), locName);
			}
			ImGui::CenteredText(timeAndLoc.c_str(), false);

			ImGui::Spacing(5);
		}

		for (auto& [response, voice, name, isPlayer, hovered] : dialogue) {
			auto speakerColor = isPlayer ? GetUserStyleColorVec4(USER_STYLE::kPlayerName) : GetUserStyleColorVec4(USER_STYLE::kSpeakerName);
			ImGui::TextColoredWrapped(speakerColor, std::format("{}: ", name).c_str());

			ImGui::SameLine();

			auto lineColor = isPlayer ? GetUserStyleColorVec4(USER_STYLE::kPlayerLine) : GetUserStyleColorVec4(USER_STYLE::kSpeakerLine);
			lineColor.w = (!isGlobalHistoryOpen || isPlayer || hovered) ? 1.0f : GetUserStyleVar(USER_STYLE::kDisabledTextAlpha);

			ImGui::TextColoredWrapped(lineColor, response.c_str());

			hovered = ImGui::IsItemHovered();

			if (ImGui::IsItemClicked() && isGlobalHistoryOpen) {
				MANAGER(GlobalHistory)->PlayVoiceline(voice);
			}

			ImGui::Spacing(3);
		}
	}
	ImGui::Unindent();
}

void Dialogue::Clear()
{
	timeStamp = 0;
	dialogue.clear();
	speakerName.clear();
}
