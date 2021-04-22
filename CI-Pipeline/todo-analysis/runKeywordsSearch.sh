#!/bin/bash

#--------------------------------------------------------------------
# File: runKeywords.sh
# Description:
#    Runs keyword analysis on all *.cpp, *.c, *.hpp and *.h files in the given path
#    Multibyte character encodings (e.g. UTF-8) are handled correctly
# Parameters:
#    $1 The root folder for the search
#    $2 The csv file for the results of the analysis
# Output:
#    The keyword analysis is written to a csv file with two columns:
#    Column 1 : Filename + line number
#    Column 2 : The TODO text
#    The coluns are separated by a pipe character

#--------------------------------------------------------------------
# DECLARATIONS

# The root folder for the analysis must be passed as a parameter to the script
ROOT_DIR=$1

# The csv file for the analysis results must be passed as the second parameter to the script
CSV_FILE=$2

# List of search words, separated by pipe character
SEARCH_WORDS="TODO|FIXME|HACK|KLUDGE|WTF|FUCK|SHIT|BULLSHIT|BOLLOCKS"

#--------------------------------------------------------------------
# KEYWORDS SEARCH

# Prepare the output csv file
rm -f $CSV_FILE
echo "Location|Keyword" >> $CSV_FILE

# Grep all files below the root folder for the keywords in the word list $SEARCH_WORDS
# Options:
#    -r recursive search
#    -i case insensitive search
#    -n show line numbers
#    -E use extended regular expression syntax (backslash before pipe character is not needed)
#    -w only match on whole words

# NOTE: The call to grep is prefixed with \ to ensure any aliases are disabled

# A while loop analyses each line from the grep, using the colons as separators (grep uses colons)
while IFS=":" read -r colFile colLine colLineWithKeyword
do
    # Get the character offset of the keyword in the line
    # tr is used to covert the line to uppercase, to perform a case-insensitive search
    # The awk match function is used to determine the character offset of the keyword in the line
    # NOTE: grep --byte-offset is not used for this, as a multibyte-character aware solution is needed (e.g. for UTF-8)
    offsetKeyword=$(echo $colLineWithKeyword | tr '[:lower:]' '[:upper:]' | awk 'END{print match($0,''"'$SEARCH_WORDS'"'')}')
        
    # Extract the text to the right of the keyword (this will usually be whatever text the developer wrote to explain his use of the keyword)
    # The second awk removed leading and trailing whitespace
    # NOTE: The awk print length function is used to get the line length as this is the only method to get line length that worked in all the cases I tested
    lengthLineWithKeyword=$(echo $colLineWithKeyword | awk '{print length}' | awk '{$1=$1};1')
    prettifiedKeyword=$(echo $colLineWithKeyword | cut -c$offsetKeyword-$lengthLineWithKeyword)

    echo "$(basename $colFile):$colLine|$prettifiedKeyword" >> $CSV_FILE
done < <(\grep -rinEw $SEARCH_WORDS --include=*.{cpp,c,hpp,h} $ROOT_DIR)
