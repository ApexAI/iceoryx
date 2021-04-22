#--------------------------------------------------------------------
# File: exportControlFlowGraph.py
# Description: Exports the control flow graph for a single function from an Understand database
# Prerequisites:
#    The Understand database should already exist
# Parameters:
#    args[1] - Path to the Understand database, *.udb
#    args[2] - Name of function to be analysed
#    args[3] - Path to the output file, *.png
# Output:
#    A control flow graph for the named function in png format

import understand
import string
import sys

# Export the control flow graph for the named function
# Parameters:
#   db - The Understand database, already open
#   funcname - The name of the function
#   pngpath - The full path to the PNG file that should be generated
def drawGraph(db, funcname, pngpath):
    # The function
    func = None
    # Get a list of all functions from the database
    allfuncs = db.lookup(funcname, "function,method,procedure")
    # Look for the correct function entity
    for f in allfuncs:
        print (f.longname(),"-",f.kindname())
        if f.longname() == funcname:
            func = f
    # Draw the diagram if the function was found
    if func is not None:
        print ("GENERATING ",func.longname(),"-",func.kindname())
        try:
            func.draw("Control Flow", pngpath)
        except understand.UnderstandError as err:
            print("Error: {0}".format(err))

if __name__ == '__main__':
    # Open Database
    args = sys.argv
    db = understand.open(args[1])
    drawGraph(db, args[2], args[3])

