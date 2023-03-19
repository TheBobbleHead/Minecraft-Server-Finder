#include <string>
#include <vector>

class StringParser {

	struct GroupingChar {
		char Char = NULL;
		bool IncludeChar = false;
	};

	struct GroupingCharPiar {
		char OpeningChar = NULL;
		char ClosingChar = NULL;

		bool IncludeOpening = false;
		bool IncludeClosing = false;

		bool operator == (GroupingCharPiar GroupingPair);
	};

private:

	std::vector<GroupingChar> GroupingChars;
	std::vector<GroupingCharPiar> GroupingCharPiars;
	std::vector<char> IgnoreChars;

public:

	std::string operator [] (const int& Index);

	StringParser(std::string* UseString);

	std::string* String;
	std::vector<std::string>* ChunkedString;

	bool IgnoreCharsEnabled = false;
	bool IgnoreSeperatorInGroup = false;
	bool IngnoreExtraSeperators = false;
	//bool SplitNestedGroups = false;

	void SeperateString(char Seperator, bool IncludeSeperator);
	void AddGroupingChar(char GroupingChar_, bool IncludeThisChar);
	void AddGroupingCharPair(char OpenChar, char ClosingChar, bool IncludeOpen, bool IncludeClose);
	void RemoveGroupingChar(char GroupingChar);
	void RemoveGroupingCharPair(char OpenChar, char ClosingChar);
	void ClearGroupingChars();
	void AddIgnoreChar(char Ignore);
	void RemoveIgnoreChar(char IgnoreRM);
	void ClearIgnoreChars();

	void ClearChunkedString();
	void ClearAll();

	~StringParser();
};