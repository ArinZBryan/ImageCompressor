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