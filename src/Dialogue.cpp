#include "Dialogue.h"

#include "GlobalHistory.h"
#include "ImGui/Styles.h"
#include "NPCNameProvider.h"

TimeStamp::TimeStamp(std::uint64_t a_timeStamp, const std::string& a_format) :
	time(a_timeStamp),
	format(std::format("{}##{}", a_format, a_timeStamp))
{}

std::uint64_t TimeStamp::GenerateTimeStamp(std::tm a_time)
{
	return REX::STR::TO_NUM<std::uint64_t>(std::format("{:03}{:02}{:02}{:02}{:02}", a_time.tm_year, a_time.tm_mon, a_time.tm_mday, a_time.tm_hour, a_time.tm_min));
}

const char* TimeStamp::GetMonthName(std::uint32_t a_month)
{
	switch (a_month) {
	case RE::Calendar::Month::kMorningStar:
		return *"sMonthJanuary"_gs;
	case RE::Calendar::Month::kSunsDawn:
		return *"sMonthFebruary"_gs;
	case RE::Calendar::Month::kFirstSeed:
		return *"sMonthMarch"_gs;
	case RE::Calendar::Month::kRainsHand:
		return *"sMonthApril"_gs;
	case RE::Calendar::Month::kSecondSeed:
		return *"sMonthMay"_gs;
	case RE::Calendar::Month::kMidyear:
		return *"sMonthJune"_gs;
	case RE::Calendar::Month::kSunsHeight:
		return *"sMonthJuly"_gs;
	case RE::Calendar::Month::kLastSeed:
		return *"sMonthAugust"_gs;
	case RE::Calendar::Month::kHearthfire:
		return *"sMonthSeptember"_gs;
	case RE::Calendar::Month::kFrostfall:
		return *"sMonthOctober"_gs;
	case RE::Calendar::Month::kSunsDusk:
		return *"sMonthNovember"_gs;
	case RE::Calendar::Month::kEveningStar:
		return *"sMonthDecember"_gs;
	default:
		return "Bad Month";
	}
}

const char* TimeStamp::GetOrdinalSuffix(std::uint32_t a_day)
{
	switch (a_day) {
	case 1:
	case 21:
	case 31:
		return *"sFirstOrdSuffix"_gs;
	case 2:
	case 22:
		return *"sSecondOrdSuffix"_gs;
	case 3:
	case 23:
		return *"sThirdOrdSuffix"_gs;
	default:
		return *"sDefaultOrdSuffix"_gs;
	}
}

std::tm TimeStamp::ExtractTimeStamp(std::uint64_t a_timeStamp)
{
	std::tm time{};

	time.tm_min = a_timeStamp % 100;
	a_timeStamp /= 100;
	time.tm_hour = a_timeStamp % 100;
	a_timeStamp /= 100;
	time.tm_mday = a_timeStamp % 100;
	a_timeStamp /= 100;
	time.tm_mon = a_timeStamp % 100;
	a_timeStamp /= 100;
	time.tm_year = static_cast<int>(a_timeStamp);

	return time;
}

std::string TimeStamp::GetFormattedYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day)
{
	return std::format("{}{}{}{}, 4E {}", a_day, GetOrdinalSuffix(a_day), *"sOf"_gs, GetMonthName(a_month), a_year);
}

std::string TimeStamp::GetFormattedHourMin(std::uint32_t a_hour, std::uint32_t a_minute, bool a_12HourFormat)
{
	if (a_12HourFormat) {
		const char* AMPM;
		if (a_hour < 12) {
			if (a_hour == 0) {
				a_hour = 12;
			}
			AMPM = *"sTimeAM"_gs;
		} else {
			if (a_hour > 12) {
				a_hour -= 12;
			}
			AMPM = *"sTimePM"_gs;
		}
		return std::format("{:02}:{:02} {}", a_hour, a_minute, AMPM);
	} else {
		return std::format("{:02}:{:02}", a_hour, a_minute);
	}
}

void TimeStamp::FromYearMonthDay(std::uint32_t a_year, std::uint32_t a_month, std::uint32_t a_day)
{
	time = REX::STR::TO_NUM<std::uint32_t>(std::format("{:03}{:02}{:02}", a_year, a_month, a_day));
	format = std::format("{}##{}", GetFormattedYearMonthDay(a_year, a_month, a_day), time);
}

void TimeStamp::FromHourMin(std::uint32_t a_hour, std::uint32_t a_minute, const std::string& a_speaker, bool a_12HourFormat)
{
	time = REX::STR::TO_NUM<std::uint32_t>(std::format("{:02}{:02}", a_hour, a_minute));
	format = std::format("{} - {}##{}", GetFormattedHourMin(a_hour, a_minute, a_12HourFormat), a_speaker, time);
}

void TimeStamp::SwitchHourFormat(bool a_12HourFormat)
{
	std::uint32_t hour = static_cast<std::uint32_t>(time / 100);
	std::uint32_t minute = time % 100;

	auto hourMin = GetFormattedHourMin(hour, minute, a_12HourFormat);
	format = std::format("{}{}", hourMin, format.substr(format.find(" - ")));
}

Speech::Speech(const std::tm& a_time, RE::TESObjectREFR* a_speaker)
{
	Initialize(a_speaker);
	Initialize(a_time);
}

Speech::Line::Line(std::string a_line, std::string a_voice) :
	line(std::move(a_line)),
	voice(std::move(a_voice))
{}

void Speech::Line::Sanitize()
{
	if (line.empty() || line == " ") {
		line = "...";
	}
}

void Speech::Initialize(RE::TESObjectREFR* a_speaker)
{
	if (!speakerName.empty()) {
		return;
	}

	if (a_speaker) {
		id.SetNumericID(a_speaker->GetFormID());
		speakerName = NPCNameProvider::GetSingleton()->GetName(a_speaker);
		if ((a_speaker->IsDynamicForm() || a_speaker->GetObjectReference() && a_speaker->GetObjectReference()->IsDynamicForm()) && speakerName.empty()) {
			tempSpeaker = speakerName;
		}
		RE::TESForm* cellOrLoc = a_speaker->GetCurrentLocation();
		if (!cellOrLoc) {
			cellOrLoc = a_speaker->GetParentCell();
		}
		if (cellOrLoc) {
			loc.SetNumericID(cellOrLoc->GetFormID());
			locName = cellOrLoc->GetName();
		}
		if (locName.empty()) {
			locName = "$DH_UnknownLocation"_T;
		}
	}
}

void Speech::Initialize(const std::tm& a_time)
{
	timeStamp = TimeStamp::GenerateTimeStamp(a_time);
}

std::tm Speech::ExtractTimeStamp() const
{
	return TimeStamp::ExtractTimeStamp(timeStamp);
}

bool Speech::ResolveIDs()
{
	auto speakerActor = RE::TESForm::LookupByID<RE::Actor>(id.GetNumericID());
	if (speakerActor) {
		speakerName = NPCNameProvider::GetSingleton()->GetName(speakerActor);
	} else if (tempSpeaker) {
		speakerName = *tempSpeaker;
	} else {
		return false;
	}

	locName = "$DH_UnknownLocation"_T;
	if (auto cellOrLoc = RE::TESForm::LookupByID(loc.GetNumericID())) {
		locName = cellOrLoc->GetName();
	}

	return true;
}

void Speech::Clear()
{
	timeStamp = 0;
	id = {};
	loc = {};
	locName.clear();
	speakerName.clear();
}

Dialogue::Dialogue(const std::tm& a_time, RE::TESObjectREFR* a_speaker) :
	Speech::Speech(a_time, a_speaker)
{}

Dialogue::Line::Line(RE::TESObjectREFR* a_speaker, std::string a_line, std::string a_voice) :
	Speech::Line::Line(std::move(a_line), std::move(a_voice)),
	isPlayer(a_speaker->IsPlayerRef())
{}

std::string Dialogue::TimeStampToString(bool a_use12HourFormat) const
{
	auto time = ExtractTimeStamp();
	return std::format("{} - {}",
		TimeStamp::GetFormattedYearMonthDay(time.tm_year, time.tm_mon, time.tm_mday),
		TimeStamp::GetFormattedHourMin(time.tm_hour, time.tm_min, a_use12HourFormat));
}

void Dialogue::AddDialogue(RE::TESObjectREFR* a_speaker, std::string a_line, std::string a_voice)
{
	Initialize(a_speaker);
	dialogue.emplace_back(a_speaker, std::move(a_line), std::move(a_voice));
}

void Dialogue::Draw()
{
	using namespace ImGui;

	auto mgr = MANAGER(GlobalHistory);

	const auto& playerName = mgr->GetPlayerName();

	if (refreshContents || nameWidth == 0.0f) {
		refreshContents = false;
		nameWidth = std::max(ImGui::CalcTextSize(playerName.c_str()).x, ImGui::CalcTextSize(speakerName.c_str()).x);
		colonWidth = ImGui::CalcTextSize(":").x;
	}

	bool isGlobalHistoryOpen = mgr->IsGlobalHistoryOpen();

	ImGui::Indent();
	{
		if (isGlobalHistoryOpen) {
			auto [headerFont, headerFontSize] = MANAGER(IconFont)->GetHeaderFont();
			ImGui::PushFont(headerFont, headerFontSize);
			{
				ImGui::CenteredText(speakerName.c_str(), false);
			}
			ImGui::PopFont();
			if (timeAndLoc.empty()) {
				timeAndLoc = std::format("{} - {}", TimeStampToString(mgr->Use12HourFormat()), locName);
			}
			ImGui::CenteredText(timeAndLoc.c_str(), false);
			ImGui::Spacing(4);
		}

		auto childSize = ImGui::GetContentRegionAvail();

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4());

		if (ImGui::BeginTable("##DialogueLogs", 3, ImGuiTableFlags_ScrollY)) {
			ImGui::TableSetupColumn("##Name", ImGuiTableColumnFlags_WidthFixed, nameWidth);
			ImGui::TableSetupColumn("##Colon", ImGuiTableColumnFlags_WidthFixed, colonWidth);
			ImGui::TableSetupColumn("##Line", ImGuiTableColumnFlags_WidthStretch);

			for (auto& line : dialogue) {
				bool        hovered = mgr->IsLineHovered(&line);
				const auto& name = line.isPlayer ? playerName : speakerName;
				auto        speakerColor = line.isPlayer ? GetUserStyleColorVec4(USER_STYLE::kPlayerName) : GetUserStyleColorVec4(USER_STYLE::kSpeakerName);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				{
					ImGui::TextColored(speakerColor, name.c_str());
				}
				ImGui::TableSetColumnIndex(1);
				{
					ImGui::TextColored(speakerColor, ":");
				}
				ImGui::TableSetColumnIndex(2);
				{
					auto lineColor = line.isPlayer ? GetUserStyleColorVec4(USER_STYLE::kPlayerLine) : GetUserStyleColorVec4(USER_STYLE::kSpeakerLine);
					lineColor.w = (!isGlobalHistoryOpen || line.isPlayer || hovered) ? 1.0f : GetUserStyleVar(USER_STYLE::kDisabledTextAlpha);

					ImGui::TextColoredWrapped(lineColor, line.line.c_str());

					mgr->SetLineHovered(&line, ImGui::IsItemHovered());

					if (ImGui::IsItemSelected() && isGlobalHistoryOpen) {
						mgr->PlayVoiceline(line.voice);
					}
				}
				ImGui::Spacing(3);
			}
			ImGui::EndTable();
		}

		ImGui::PopStyleColor();
	}
	ImGui::Unindent();
}

void Dialogue::Clear()
{
	Speech::Clear();

	dialogue.clear();
	timeAndLoc.clear();
	nameWidth = 0.0f;
	colonWidth = 0.0f;

	RefreshContents();
}

void Dialogue::RefreshContents()
{
	refreshContents = true;
}

bool Dialogue::Resolve()
{
	if (!ResolveIDs()) {
		return false;
	}
	for (auto& line : dialogue) {
		line.isPlayer = line.voice.empty();
		line.Sanitize();
	}
	return true;
}

Monologue::Monologue(std::tm& a_time, RE::TESObjectREFR* a_speaker, std::string a_line, std::string a_voice, RE::TESTopic* a_topic) :
	Speech::Speech(a_time, a_speaker),
	line(std::move(a_line), std::move(a_voice))
{
	topic.SetNumericID(a_topic ? a_topic->GetFormID() : 0);
	if (a_topic) {
		dialogueType = a_topic->data.type.underlying();
	}
}

bool Monologue::Resolve()
{
	if (!ResolveIDs()) {
		return false;
	}
	if (auto topicForm = RE::TESForm::LookupByID<RE::TESTopic>(topic.GetNumericID())) {
		dialogueType = topicForm->data.type.underlying();
	}
	line.Sanitize();
	return true;
}

void Monologues::Draw()
{
	auto mgr = MANAGER(GlobalHistory);
	bool use12HourFormat = mgr->Use12HourFormat();

	if (refreshContents || timeWidth == 0.0f || nameWidth == 0.0f) {
		refreshContents = false;

		nameWidth = 0.0f;
		timeWidth = ImGui::CalcTextSize(use12HourFormat ? "88:88 AM" : "88:88").x;

		std::unordered_set<std::string_view> names{};
		for (auto& monologue : monologues) {
			if (names.insert(monologue->speakerName).second) {
				if (auto width = ImGui::CalcTextSize(monologue->speakerName.c_str()).x; width > nameWidth) {
					nameWidth = width;
				}
			}
		};

		colonWidth = ImGui::CalcTextSize(":").x;
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4());

	if (ImGui::BeginTable("##Logs", 4, ImGuiTableFlags_ScrollY)) {
		ImGui::TableSetupColumn("##Time", ImGuiTableColumnFlags_WidthFixed, timeWidth);
		ImGui::TableSetupColumn("##Name", ImGuiTableColumnFlags_WidthFixed, nameWidth);
		ImGui::TableSetupColumn("##Colon", ImGuiTableColumnFlags_WidthFixed, colonWidth);
		ImGui::TableSetupColumn("##Line", ImGuiTableColumnFlags_WidthStretch);

		auto speakerColor = ImGui::GetUserStyleColorVec4(ImGui::USER_STYLE::kSpeakerName);

		for (auto& monologuePtr : monologues | std::views::reverse) {
			auto& monologue = *monologuePtr;
			bool  hovered = mgr->IsLineHovered(&monologue.line);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			{
				if (monologue.hourMinTimeStamp.empty()) {
					auto tm = monologue.ExtractTimeStamp();
					monologue.hourMinTimeStamp = TimeStamp::GetFormattedHourMin(tm.tm_hour, tm.tm_min, use12HourFormat);
				}
				ImGui::Text(monologue.hourMinTimeStamp.c_str());
			}
			auto& [response, voice] = monologue.line;
			ImGui::TableSetColumnIndex(1);
			{
				ImGui::TextColored(speakerColor, monologue.speakerName.c_str());
			}
			ImGui::TableSetColumnIndex(2);
			{
				ImGui::TextColored(speakerColor, ":");
			}
			ImGui::TableSetColumnIndex(3);
			{
				auto lineColor = GetUserStyleColorVec4(ImGui::USER_STYLE::kSpeakerLine);
				lineColor.w = hovered ? 1.0f : GetUserStyleVar(ImGui::USER_STYLE::kDisabledTextAlpha);
				ImGui::TextColoredWrapped(lineColor, response.c_str());
				mgr->SetLineHovered(&monologue.line, ImGui::IsItemHovered());
				if (ImGui::IsItemSelected()) {
					mgr->PlayVoiceline(voice);
				}
			}
			ImGui::Spacing(3);
		}
		ImGui::EndTable();
	}

	ImGui::PopStyleColor();
}

void Monologues::RefreshContents()
{
	refreshContents = true;
}

bool Monologues::empty() const
{
	return monologues.empty();
}

void Monologues::clear()
{
	monologues.clear();
}
