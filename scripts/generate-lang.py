#!/usr/bin/python3

import sys
import xml.etree.ElementTree as ElementTree

import definitions

def appendContextTag(element, idValue, styleRefValue):
  newElement = ElementTree.SubElement(element, "context")
  newElement.set("id", idValue)
  newElement.set("style-ref", styleRefValue)

  return newElement

def appendKeywordTag(element, keyword):
  newElement = ElementTree.SubElement(element, "keyword")
  newElement.text = keyword

#Take the base input, the header input and output path
if (len(sys.argv) < 4):
  print("Expected two input files and an output file")
  exit(1)

inputFile = sys.argv[1]
headerFile = sys.argv[2]
outputFile = sys.argv[3]

#Parse the template and find the element to modify
languageTree = ElementTree.parse(inputFile)
includeElement = languageTree.find(".//include");

#Create tags for the instructions and registers
instructionsTag = appendContextTag(includeElement, "instructions", "keyword")

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
