#!/bin/bash

#--------------------------------------------------------------------
# File: axivionViolationReportCsv.sh
# Description:
#    Analyses an axivion SQlite database and generates CSV files for different kinds of violations:
#        Autosar C++
#        Misra C++
#        Cert C++
#        CWE
# Prerequisites:
# Output:
#    A separate csv file is written for each violation type. Two columns, separated by a pipe.

#--------------------------------------------------------------------
# DECLARATIONS

# The path to the SQlite database file must be passed as a parameter to the script
AXIVION_DB_PATH=$1

# The output folder for the csv files must be passed as a parameter to the script
OUTPUT_DIR=$2

# Output filename
AUTOSAR_CPP_VIOLATIONS_PATH=$OUTPUT_DIR/AutosarCppViolations.csv
MISRA_CPP_VIOLATIONS_PATH=$OUTPUT_DIR/MisraCppViolations.csv
CWE_VIOLATIONS_PATH=$OUTPUT_DIR/CWEViolations.csv
CERT_CPP_VIOLATIONS_PATH=$OUTPUT_DIR/CertCppViolations.csv

#--------------------------------------------------------------------

# Add a view to the database that presents the style violations in an easier format
sqlite3 $AXIVION_DB_PATH <<EOF
DROP VIEW IF EXISTS ccStyleViolations;
CREATE VIEW ccStyleViolations AS
SELECT axStyleViolationRef.ID AS ID,
       axSLoc.Filename,
       axSLoc.Line,
       axObjectname.Name As Objectname,
       ErrnoString.Name AS Errno,
       MessageString.Name AS Message
FROM axStyleViolationRef
INNER JOIN axObjectname ON axStyleViolationRef.Objectname_ID = axObjectname.ID
INNER JOIN axStyleViolation_Version ON axStyleViolationRef.ID = axStyleViolation_Version.ID
INNER JOIN axSLoc ON axStyleViolation_Version.SLoc_ID = axSLoc.ID
INNER JOIN axString AS ErrnoString ON axStyleViolationRef.Errno_String_ID = ErrnoString.ID
INNER JOIN axString AS MessageString ON axStyleViolationRef.Message_String_ID = MessageString.ID
EOF

# Generate the output files
# sqlite3 outputs the SELECT results with columns separated by pipe character.

echo "Rule|Location" >> $AUTOSAR_CPP_VIOLATIONS_PATH
sqlite3 $AXIVION_DB_PATH "SELECT Errno || '|' || Filename || ':' || Line FROM ccStyleViolations WHERE Errno LIKE 'AutosarC++%' ORDER BY Filename" >> $AUTOSAR_CPP_VIOLATIONS_PATH

echo "Rule|Location" >> $MISRA_CPP_VIOLATIONS_PATH
sqlite3 $AXIVION_DB_PATH "SELECT Errno || '|' || Filename || ':' || Line FROM ccStyleViolations WHERE Errno LIKE 'MisraC++%' ORDER BY Filename" >> $MISRA_CPP_VIOLATIONS_PATH

echo "Rule|Location" >> $CWE_VIOLATIONS_PATH
sqlite3 $AXIVION_DB_PATH "SELECT Errno || '|' || Filename || ':' || Line FROM ccStyleViolations WHERE Errno LIKE 'CWE%' ORDER BY Filename" >> $CWE_VIOLATIONS_PATH

echo "Rule|Location" >> $CERT_CPP_VIOLATIONS_PATH
sqlite3 $AXIVION_DB_PATH "SELECT Errno || '|' || Filename || ':' || Line FROM ccStyleViolations WHERE Errno LIKE 'CertC++%' ORDER BY Filename" >> $CERT_CPP_VIOLATIONS_PATH
