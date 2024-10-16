#pragma once
#include "framework.h"
#include <optional>
#include <variant>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstring>
#include <cerrno>
#include <iostream>

struct CLIArg {
	char shortName[3];			//-[any char]
	const char* longName;		//--[any continuous name without spaces]
	const char* description;	//[any string]
	std::variant<std::optional<bool>, std::optional<int>, std::optional<std::string>> value;
	bool required;
};

std::string cliargtoa(CLIArg arg);

std::unordered_map<std::string, CLIArg> parseArgs(int argc, const char** argv, int argLen, const CLIArg* argList);
