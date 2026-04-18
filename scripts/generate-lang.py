#!/usr/bin/python3

import sys
import xml.etree.ElementTree as ElementTree

import definitions

def appendKeywordTag(element, keyword):
  newElement = ElementTree.SubElement(element, "keyword")
  newElement.text = keyword

def findInstructionsTag(tree):
  elements = tree.findall(".//context");
  for element in elements:
    if element.get("id") == "instructions":
      return element

#Take the base input, the header input and output path
if (len(sys.argv) < 4):
  print("Expected two input files and an output file")
  exit(1)

inputFile = sys.argv[1]
headerFile = sys.argv[2]
outputFile = sys.argv[3]

#Parse the template and find the instructions tag to modify
languageTree = ElementTree.parse(inputFile)
instructionsTag = findInstructionsTag(languageTree)

#Append the instructions
for instructionGroup in definitions.instructionGroups:
  for instruction in instructionGroup:
    appendKeywordTag(instructionsTag, instruction)

#Read in the header
headerText = ""
with open(headerFile, "r") as file:
  headerText = file.read()

#Write the header and built language to the output
outputBytes = ElementTree.tostring(languageTree.getroot(), encoding = "utf8", xml_declaration = False)
with open(outputFile, "w") as file:
  file.write(headerText)
  file.write(outputBytes.decode("utf-8"))
