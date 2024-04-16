import os

# Arguments
import argparse
parser=argparse.ArgumentParser(description="Concatenates a similar CSV layout to a file")
parser.add_help=True
parser.add_argument("data", help="Path to an intermediate file to append")
parser.add_argument("path", help="Path to the output file")
parser.add_argument("appendHeader", help="The extra header name to append")
parser.add_argument("appendData", help="The extra data to append under a new header")

args=parser.parse_args()

headers=0
# If the file does not exist, headers must be kept
if not os.path.isfile(args.path):
    headers=1

inF = open(args.data, "r")
data = inF.read()
inF.close()

outF = open(args.path, "a")

firstLine=1
for line in data.splitlines():
    if bool(firstLine): # This is the CSV headers
        firstLine=0
        if bool(headers): # Write headers once only
            outF.write(args.appendHeader)
            outF.write(',')
            outF.write(line)
        else:
            continue    
    else:
        outF.write(args.appendData)
        outF.write(',')
        outF.write(line)
    outF.write('\n')
    
outF.close();