#!/bin/bash

#--------------------------------------------------------------------
# File: axivionCyclesReportRst.sh
# Description:
#    Analyses an axivion SQlite database for cycle violations
# Prerequisites:
# Output:
#    RST file that can be included in a Sphinx document

#--------------------------------------------------------------------
# DECLARATIONS

# The path to the SQlite database file must be passed as a parameter to the script
AXIVION_DB_PATH=$1
echo "AXIVION_DB_PATH=$AXIVION_DB_PATH"

# The document root folder must be passed as a parameter to the script
DOCUMENT_ROOT_DIR=$2
echo "DOCUMENT_ROOT_DIR=$DOCUMENT_ROOT_DIR"

# The folder under the document root where the axivion assets files will be put must be passed as a parameter to the script
AXIVION_ASSETS_SUBDIR=$3
echo "AXIVION_ASSETS_SUBDIR=$AXIVION_ASSETS_SUBDIR"

# The folder where the rst file and images will be placed
OUTPUT_DIR=$DOCUMENT_ROOT_DIR/$AXIVION_ASSETS_SUBDIR
echo "OUTPUT_DIR=$OUTPUT_DIR"

# The RST file
CYCLES_REPORT_RST=$OUTPUT_DIR/AxivionCyclesReport.rst

#--------------------------------------------------------------------

# Delete the output file if it exists
[ -f "$CYCLES_REPORT_RST" ] && rm $CYCLES_REPORT_RST

# Ensure the output folder exists
mkdir -p $DOCUMENT_ROOT_DIR/$AXIVION_ASSETS_SUBDIR

# Add a view to the database that presents the cycle violations in an easier format
sqlite3 $AXIVION_DB_PATH <<EOF
DROP VIEW IF EXISTS ccCycles;
CREATE VIEW ccCycles AS
SELECT axCycle.ID AS ID,
       axSLoc.Filename,
       axSLoc.Line,
       SourceEntity.Objectname AS SourceName,
	   SourceEntity.EntityType AS SourceType,
       TargetEntity.Objectname AS TargetName,
       TargetEntity.EntityType AS TargetType,
       axCycle_Version.Image_ID AS ImageID
FROM axCycle
INNER JOIN axCycle_Version ON axCycle.ID = axCycle_Version.ID
INNER JOIN axDependency_Version ON axCycle.Dependency_ID = axDependency_Version.Dependency_ID
INNER JOIN axDependency ON axCycle.Dependency_ID = axDependency.ID
INNER JOIN axSLoc ON axDependency_Version.SLoc_ID = axSLoc.ID
INNER JOIN axEntity AS SourceEntity ON axDependency.Source_Entity_ID = SourceEntity.ID
INNER JOIN axEntity AS TargetEntity ON axDependency.Target_Entity_ID = TargetEntity.ID
EOF

# The number of cycle violations is the number of generated images
NUM_CYCLE_VIOLATIONS=$(sqlite3 $AXIVION_DB_PATH "SELECT MAX(ImageID) FROM ccCycles")

# Process the cycle violations
for ((i=1;i<=$NUM_CYCLE_VIOLATIONS;i++)); do
   if [[ $(sqlite3 $AXIVION_DB_PATH "SELECT SourceType FROM ccCycles WHERE ImageID = $i LIMIT 1") == "File" ]]; then
      # Include cycle found
      echo -e "Include Cycle:\n" >> $CYCLES_REPORT_RST
      # Each line starts with a pipe which will be interpreted as a list element by sphinx
      sqlite3 $AXIVION_DB_PATH "SELECT '   | ' || Filename FROM ccCycles WHERE ImageID = $i" >> $CYCLES_REPORT_RST
      echo -e "\n" >> $CYCLES_REPORT_RST
   else
      # Callgraph cycle found
      
      # Extract the image file (stored in the database as compressed svg)
      IMAGE_FILENAME="CycleViolation_"$i
      sqlite3 $AXIVION_DB_PATH "SELECT writefile('"$IMAGE_FILENAME".gz', Image) FROM axImage WHERE ID = $i"
      gzip -d $IMAGE_FILENAME.gz
      mv $IMAGE_FILENAME $OUTPUT_DIR/$IMAGE_FILENAME.svg
      
      # Generate the document
      echo -e "Callgraph Cycle:\n" >> $CYCLES_REPORT_RST
      echo -e ".. csv-table:: Callgraph Cycle" >> $CYCLES_REPORT_RST
      echo -e "   :header-rows: 1" >> $CYCLES_REPORT_RST
      echo -e "   :widths: 50, 25, 25" >> $CYCLES_REPORT_RST
      echo -e "   :delim: |\n" >> $CYCLES_REPORT_RST
      echo -e "   Location|Source|Target" >> $CYCLES_REPORT_RST
      sqlite3 $AXIVION_DB_PATH "SELECT DISTINCT '   ' || Filename || ':' || Line || '|' || SourceName || '|' || TargetName FROM ccCycles WHERE ImageID = $i" >> $CYCLES_REPORT_RST
      echo -e "\n" >> $CYCLES_REPORT_RST
      echo -e ".. image:: $AXIVION_ASSETS_SUBDIR/$IMAGE_FILENAME.svg\n" >> $CYCLES_REPORT_RST
   fi
done
