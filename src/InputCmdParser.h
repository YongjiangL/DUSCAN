/*
 * InputCmdParser.h
 *
 *  Created on: Oct 8, 2016
 *      Author: torkit
 */

#ifndef INPUTCMDPARSER_H_
#define INPUTCMDPARSER_H_

#include <algorithm>
#include <string>

using namespace std;

namespace SCAN {

class InputCmdParser {
public:
	static char* getCmdOption(int argc, char *argv[], const string &option);
	static bool cmdOptionExists(int argc, char *argv[], const string &option);
};

} /* namespace SCAN */
#endif /* INPUTCMDPARSER_H_ */
