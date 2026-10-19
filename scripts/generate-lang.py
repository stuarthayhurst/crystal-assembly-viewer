#!/usr/bin/python3

import sys
import xml.etree.ElementTree as ElementTree

import definitions_x86
import definitions_arm

def setArchitecture(tree, architecture):
  #Set language tag attributes
  languageTree.getroot().set("id", f"assembler-{architecture}")
  languageTree.getroot().set("name", f"Assembler ({architecture} compiler)")

  #Set the top-level context tag's ID
  elements = languageTree.findall(".//context");
  for element in elements:
    if element.get("id") == "assembler-arch":
      element.set("id", f"assembler-{architecture}")
      break

def findContextTag(tree, tagId):
  elements = tree.findall(".//context");
  for element in elements:
    if element.get("id") == tagId:
      return element

def getParentElement(tree, element):
  for target in tree.iter():
    for child in target:
      if child == element:
        return target

  return None

def getElementIndex(parent, element):
  index = 0
  for target in parent:
    if target == element:
      return index
    index += 1

  return -1

def appendTag(element, tagName, tagContent):
  newElement = ElementTree.SubElement(element, tagName)
  newElement.text = tagContent

  return newElement

insertedCount = 0
def appendGroups(tree, element, groups):
  global insertedCount

  #Append each group, adding tags of its type
  for group in groups:
    #Get the group's tag from the group
    groupTagType = group[0]

    #Append the group based on tag type
    if groupTagType == "match":
      groupLangType = group[1]

      #Add the remainder of the group to a new context tag
      #Required since only one match can exist per context tag
      for groupEntry in group[2:]:
        #Find the parent of the node that would've been appended to
        parent = getParentElement(tree, element)
        index = getElementIndex(parent, element)

        #Create a new context element
        newElement = ElementTree.Element("context")
        newElement.set("id", f"{groupLangType}-{insertedCount}")
        newElement.set("style-ref", f"{groupLangType}")
        insertedCount += 1

        #Add the group element and insert the context element
        appendTag(newElement, groupTagType, groupEntry)
        parent.insert(index, newElement)
    else:
      for groupEntry in group[1:]:
        appendTag(element, groupTagType, groupEntry)

def cleanEmpty(tree, element):
  #Remove the element if it has no children
  if len(list(element)) == 0:
    parent = getParentElement(tree, element)
    parent.remove(element)

#Take the base input, the header input and output path
if (len(sys.argv) < 5):
  print("Expected two input files, an architecture, and an output file")
  exit(1)

#Map arguments
inputFile = sys.argv[1]
headerFile = sys.argv[2]
architecture = sys.argv[3]
outputFile = sys.argv[4]

#Select and architecture
definitions = None
if (architecture == "x86"):
  definitions = definitions_x86
elif (architecture == "arm"):
  definitions = definitions_arm
else:
  printf(f"Unknown architecture '{architecture}'")
  exit(1)

#Parse the template and substitute the architecture placeholders
languageTree = ElementTree.parse(inputFile)
setArchitecture(languageTree, architecture)

#Find tags to modify
instructionsTag = findContextTag(languageTree, "instructions-arch")
directivesTag = findContextTag(languageTree, "directives-arch")
registersTag = findContextTag(languageTree, "registers-arch")

#Append the groups
appendGroups(languageTree, instructionsTag, definitions.instructionGroups)
appendGroups(languageTree, directivesTag, definitions.directiveGroups)
appendGroups(languageTree, registersTag, definitions.registerGroups)

#Remove registers placeholder if empty
cleanEmpty(languageTree, registersTag)

#Remove whitespace
for tag in languageTree.iter():
    if tag.text is not None:
        tag.text = tag.text.strip()
    if tag.tail is not None:
        tag.tail = tag.tail.strip()

#Read in the header
headerText = ""
with open(headerFile, "r") as file:
  headerText = file.read()

#Write the header and built language to the output
outputBytes = ElementTree.tostring(languageTree.getroot(), encoding = "utf8", xml_declaration = False)
with open(outputFile, "w") as file:
  file.write(headerText)
  file.write(outputBytes.decode("utf-8"))
