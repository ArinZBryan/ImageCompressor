#include "libCLI.h"

std::string cliargtoa(CLIArg arg) {
	std::string ret;
	ret.append(arg.longName);
	ret.append(" ("); ret.append(arg.shortName); ret.append(")\n");
	ret.append("Description: "); ret.append(arg.description);; ret.append("\n");
	ret.append("Value: ");
	if (std::holds_alternative<std::optional<bool>>(arg.value)) {
		auto optional = std::get<std::optional<bool>>(arg.value);
		if (optional.has_value()) { ret.append(optional.value() ? "true" : "false"); }
		else { ret.append("N/A"); }
	}
	if (std::holds_alternative<std::optional<int>>(arg.value)) {
		auto optional = std::get<std::optional<int>>(arg.value);
		if (optional.has_value()) { ret.append(std::to_string(optional.value())); }
		else { ret.append("N/A"); }
	}
	if (std::holds_alternative<std::optional<std::string>>(arg.value)) {
		auto optional = std::get<std::optional<std::string>>(arg.value);
		if (optional.has_value()) { ret.append(optional.value()); }
		else { ret.append("N/A"); }
	}
	ret.append("\n");
	return ret;
}

// TODO: This shit doesn't fit the brief.
// This function should return a std::unordered_map of _all_ the CLIArgs, with the optional fields set as such.

/*	Example Implementation:
*		Consider making the input a std::unordered_map<std::string, CLIArg>, where all the optional fields are set to null.
*		
		This is functionally identical to the current setup, except that the array is swapped out for a std::unordered_map.
		Each key is the fully qualified --name, and the values are the same as they are now.
*		
		This function then is mostly the same, except that it returns void, modifying the existing std::unordered_map passed
*		to it. Instead of creating a new map, and adding any found arguments, we check first to see if the fully qualified 
*		name is in argv, and if not, then we check for the shorthand. If it is, then we perform the same parsing already here.
*		Then, if a match is found, we modify the existing value with the value passed in by the user. We don't return anything,
*		just modify the existing map. Perhaps this could be a std::unordered_map<std::string, CLIArg>& ? A pointer seems a
*		little overkill here?
*/

std::unordered_map<std::string, CLIArg> parseArgs(int argc, const char** argv, int argLen, const CLIArg* argList) {
	std::unordered_map<std::string, CLIArg> ret;
	for (int argumentNo = 0; argumentNo < argc; argumentNo++) {
		for (int knownArgNo = 0; knownArgNo < argLen; knownArgNo++) {
			if (std::strcmp(argv[argumentNo], argList[knownArgNo].shortName) == 0 || std::strcmp(argv[argumentNo], argList[knownArgNo].longName) == 0)
			{
				if (std::holds_alternative<std::optional<bool>>(argList[knownArgNo].value)) {
					CLIArg copy = argList[knownArgNo];
					copy.value.emplace<std::optional<bool>>(true);
					ret.insert({ copy.longName, copy });
				}
				else {
					argumentNo++;
					if (std::holds_alternative<std::optional<int>>(argList[knownArgNo].value)) {
						char* end;
						errno = 0;
						long val = strtol(argv[argumentNo], &end, 10);
						if (end == argv[argumentNo]) {
							std::cerr << "Argument " << argList[knownArgNo].longName << "Requires a value of type: integer" << std::endl;
							return std::unordered_map<std::string, CLIArg>();
						}
						if (end == argv[argumentNo] || *end != '\0' || errno == ERANGE) {
							std::cerr << "Invalid integer value for argument: " << argv[argumentNo - 1] << std::endl;
							return std::unordered_map<std::string, CLIArg>();
						}
						CLIArg copy = argList[knownArgNo];
						copy.value.emplace<std::optional<int>>(atoi(argv[argumentNo]));
						ret.insert({ copy.longName, copy });
					}
					if (std::holds_alternative<std::optional<std::string>>(argList[knownArgNo].value)) {
						std::string argstring = std::string(argv[argumentNo]);
						if (argv[argumentNo][0] == '"' && argv[argumentNo][strlen(argv[argumentNo]) - 1] != '"') {
							do {
								argumentNo++;
								argstring.append(argv[argumentNo]);
							} while (argv[argumentNo][strlen(argv[argumentNo]) - 1] != '"');
						}
						if (argstring[0] == '"' && argstring[argstring.length()] == '"') {
							argstring = argstring.substr(1, argstring.length() - 2);
						}
						if (argstring.length() == 0) {
							std::cerr << "Argument " << argList[knownArgNo].longName << "Requires a value of type: string" << std::endl;
							return std::unordered_map<std::string, CLIArg>();
						}
						CLIArg copy = argList[knownArgNo];
						copy.value.emplace<std::optional<std::string>>(argstring);
						ret.insert({ copy.longName, copy });
					}
				}
			}
		}
	}
	return ret;
}