#!/bin/bash

#--------------------------------------------------------------------
# File: understandCreateDatabase.sh
# Description:
#    Generates an Understand database for a software project
# Prerequisites:
# Output:
#    The Understand database

#--------------------------------------------------------------------
# DECLARATIONS

# The path to the Understand database file must be passed as a parameter to the script
UNDERSTAND_DB_PATH=$1
echo "UNDERSTAND_DB_PATH=$UNDERSTAND_DB_PATH"

# The path to the compile_commands.json file must be passed as a parameter to the script
COMPILE_COMMANDS_JSON_PATH=$2
echo "COMPILE_COMMANDS_JSON_PATH=$COMPILE_COMMANDS_JSON_PATH"

#--------------------------------------------------------------------
# DO UNDERSTAND ANALYSIS

# Get the list of system include paths for the installed g++ compiler
# The command 'g++ -E -x c++ -v /dev/null' outputs a lot of information about gcc
# The system include paths are given in the command output between the lines $LIST_START and $LIST_END
# The first sed command extracts lines between the start and end pattern, but includes the patterns
# The second sed command removes the first and last line, which at this point are the LIST_START and LIST_END lines
# tr is used to convert new lines to spaces, to give a list of paths on one line
# The final output is assigned to the script variable SYSTEM_INCLUDE_LIST
LIST_START='#include <...> search starts here:'
LIST_END='End of search list.'
SYSTEM_INCLUDES_LIST=$(g++ -E -x c++ -v /dev/null |& sed -n "/$LIST_START/,/$LIST_END/p" | sed '1d;$d' | tr '\n' ' ')
#echo "$SYSTEM_INCLUDES_LIST"

# Create an Understand database for the module and generate the required metrics
und -db $UNDERSTAND_DB_PATH create -languages C++ add $COMPILE_COMMANDS_JSON_PATH settings -c++AddFoundFilesToProject on
und -db $UNDERSTAND_DB_PATH settings -c++IncludesAdd $SYSTEM_INCLUDES_LIST
und -db $UNDERSTAND_DB_PATH analyze
