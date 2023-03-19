#include "StringParser.hpp"

bool StringParser::GroupingCharPiar::operator == (StringParser::GroupingCharPiar GroupingPair) {
	if (this->OpeningChar != GroupingPair.OpeningChar) {
		return false;
	}

	if (this->ClosingChar != GroupingPair.ClosingChar) {
		return false;
	}

	return true;
}

StringParser::StringParser(std::string* UseString) {
	String = UseString;
	ChunkedString = new std::vector<std::string>;
	return;
}

void StringParser::SeperateString(char Seperator, bool IncludeSeperator) {
	std::string CurrentChunk;
	std::vector<bool> InsudeGroup;
	std::vector<bool> InsideGroupPair;

	for (int i = 0; i < GroupingCharPiars.size(); i++) {
		InsideGroupPair.push_back(false);
	}

	for (int i = 0; i < GroupingChars.size(); i++) {
		InsudeGroup.push_back(false);
	}

	char LastReadChar = NULL;

	for (int i = 0; i < String->length(); i++) {

		for (int a = 0; a < InsudeGroup.size(); a++) {
			if (String->at(i) == GroupingChars.at(a).Char) {

				if (InsudeGroup.at(a) == true) {
					InsudeGroup.at(a) = false;
				}
				else {
					InsudeGroup.at(a) = true;
				}
			}
		}

		for (int a = 0; a < InsideGroupPair.size(); a++) {
			if (String->at(i) == GroupingCharPiars.at(a).OpeningChar) {
				InsideGroupPair.at(a) = true;
			}
			else if (String->at(i) == GroupingCharPiars.at(a).ClosingChar) {
				InsideGroupPair.at(a) = false;
			}
		}

		if (String->at(i) != Seperator) {
			bool IsGroupIgnore = false;
			bool IsInAGroup = false;

			for (int a = 0; a < GroupingChars.size(); a++) {
				if (String->at(i) == GroupingChars.at(a).Char) {

					if ((InsudeGroup.size() > 1) && (a > 0)) {
						if (InsudeGroup.at(a - 1) == true) {
							IsInAGroup = true;
						}
					}

					for (int b = 0; b < InsideGroupPair.size(); b++) {
						if (InsideGroupPair.at(b) == true) {
							IsInAGroup = true;
						}
					}
				}
			}

			for (int a = 0; a < GroupingCharPiars.size(); a++) {
				if (String->at(i) == GroupingCharPiars.at(a).OpeningChar) {

					if ((InsideGroupPair.size() > 1) && (a > 0)) {
						if (InsideGroupPair.at(a - 1) == true) {
							IsInAGroup = true;
						}
					}

					for (int b = 0; b < InsudeGroup.size(); b++) {
						if (InsudeGroup.at(b) == true) {
							IsInAGroup = true;
						}
					}
				}
			}

			if (!IsInAGroup) {

				for (int a = 0; a < GroupingChars.size(); a++) {
					if ((String->at(i) == GroupingChars.at(a).Char) && (GroupingChars.at(a).IncludeChar == false)) {
						IsGroupIgnore = true;
						break;
					}
				}

				for (int a = 0; a < GroupingCharPiars.size(); a++) {
					if ((String->at(i) == GroupingCharPiars.at(a).OpeningChar) && (GroupingCharPiars.at(a).IncludeOpening == false)) {
						IsGroupIgnore = true;
					}
				}

				for (int a = 0; a < GroupingCharPiars.size(); a++) {
					if ((String->at(i) == GroupingCharPiars.at(a).ClosingChar) && (GroupingCharPiars.at(a).IncludeClosing == false)) {
						IsGroupIgnore = true;
					}
				}
			}

			if (IsGroupIgnore == true) {
				continue;
			}

			bool IsToBeIgnored = false;

			for (int a = 0; a < IgnoreChars.size(); a++) {
				if (IgnoreChars.at(a) == String->at(i)) {
					if (IgnoreCharsEnabled) {
						IsToBeIgnored = true;
					}
				}
			}

			if (IsToBeIgnored == false) {
				CurrentChunk.push_back(String->at(i));
			}
		}
		else {
			if (LastReadChar == String->at(i)) {
				LastReadChar = String->at(i);
				continue;
			}

			bool InsideAgroup = false;

			for (int a = 0; a < InsudeGroup.size(); a++) {
				if (InsudeGroup.at(a)) {
					if (IgnoreSeperatorInGroup == false) {
						CurrentChunk.push_back(String->at(i));
					}
					InsideAgroup = true;
					break;
				}
			}

			if (!InsideAgroup) {
				for (int a = 0; a < InsideGroupPair.size(); a++) {
					if (InsideGroupPair.at(a)) {
						if (IgnoreSeperatorInGroup == false) {
							CurrentChunk.push_back(String->at(i));
						}
						InsideAgroup = true;
						break;
					}
				}
			}

			if (!InsideAgroup) {
				if (IncludeSeperator) {
					CurrentChunk.push_back(String->at(i));
					ChunkedString->push_back(CurrentChunk);
					CurrentChunk.clear();
				}
				else {
					ChunkedString->push_back(CurrentChunk);
					CurrentChunk.clear();
				}
			}
		}

		LastReadChar = String->at(i);
	}

	if (!CurrentChunk.empty()) {
		ChunkedString->push_back(CurrentChunk);
		CurrentChunk.clear();
	}

	return;
}

void StringParser::AddGroupingChar(char GroupingChar_, bool IncludeThisChar) {
	for (int i = 0; i < GroupingChars.size(); i++) {
		if ((GroupingChars.at(i).Char == GroupingChar_) && (GroupingChars.at(i).IncludeChar == IncludeThisChar)) {
			return;
		}
		else if (GroupingChars.at(i).Char == GroupingChar_) {
			GroupingChars.at(i).IncludeChar = IncludeThisChar;
			return;
		}
	}

	GroupingChar NewGroupChar;
	NewGroupChar.Char = GroupingChar_;
	NewGroupChar.IncludeChar = IncludeThisChar;
	GroupingChars.push_back(NewGroupChar);

	return;
}

void StringParser::AddGroupingCharPair(char OpenChar, char ClosingChar, bool IncludeOpen, bool IncludeClose) {
	GroupingCharPiar AddPair;
	AddPair.OpeningChar = OpenChar;
	AddPair.ClosingChar = ClosingChar;
	AddPair.IncludeOpening = IncludeOpen;
	AddPair.IncludeClosing = IncludeClose;

	for (int i = 0; i < GroupingCharPiars.size(); i++) {
		if (GroupingCharPiars.at(i) == AddPair) {
			GroupingCharPiars.at(i).IncludeClosing = IncludeClose;
			GroupingCharPiars.at(i).IncludeOpening = IncludeOpen;
			return;
		}
	}

	GroupingCharPiars.push_back(AddPair);
	return;
}

void StringParser::RemoveGroupingChar(char GroupingChar) {
	for (int i = 0; i < GroupingChars.size(); i++) {
		if (GroupingChars.at(i).Char == GroupingChar) {
			GroupingChars.erase(GroupingChars.begin() + i);
			return;
		}
	}

	return;
}

void StringParser::RemoveGroupingCharPair(char OpenChar, char ClosingChar) {
	GroupingCharPiar Input;
	Input.OpeningChar = OpenChar;
	Input.ClosingChar = ClosingChar;

	for (int i = 0; i < GroupingCharPiars.size(); i++) {
		if (GroupingCharPiars.at(i) == Input) {
			GroupingCharPiars.erase(GroupingCharPiars.begin() + i);
			return;
		}
	}
	return;
}

void StringParser::ClearGroupingChars() {
	GroupingChars.clear();
}

void StringParser::AddIgnoreChar(char Ignore) {
	for (int i = 0; i < IgnoreChars.size(); i++) {
		if (IgnoreChars.at(i) == Ignore) {
			return;
		}
	}

	IgnoreChars.push_back(Ignore);

	return;
}

void StringParser::RemoveIgnoreChar(char IgnoreRM) {
	for (int i = 0; i < IgnoreChars.size(); i++) {
		if (IgnoreChars.at(i)) {
			IgnoreChars.erase(IgnoreChars.begin() + i);
			return;
		}
	}
}

void StringParser::ClearIgnoreChars() {
	IgnoreChars.clear();
}

void StringParser::ClearChunkedString() {
	ChunkedString->clear();
}

void StringParser::ClearAll() {
	ClearIgnoreChars();
	ClearChunkedString();
	*String = "";
	ClearGroupingChars();

	IgnoreCharsEnabled = false;
	IgnoreSeperatorInGroup = false;
	IngnoreExtraSeperators = false;
}

StringParser::~StringParser() {
	//delete chunked string
	delete ChunkedString;
	return;
}

std::string StringParser::operator[] (const int& Index) {
	std::string ReturnString;
	ReturnString = this->ChunkedString->at(Index);
	return ReturnString;
}