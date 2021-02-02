/*
 * InputCmdParser.cpp
 *
 *  Created on: Oct 8, 2016
 *      Author: torkit
 */

#include "InputCmdParser.h"

namespace PROB {

char *InputCmdParser::getCmdOption(int argc, char *argv[], const string &option) {
	char **itr = std::find(argv, argv+argc, option);
	if (itr != argv + argc && ++itr != argv + argc)
		return *itr;
	return 0;
}

bool InputCmdParser::cmdOptionExists(int argc, char *argv[], const string &option) {
	return std::find(argv, argv+argc, option) != argv+argc;
}

} /* namespace PROB */
