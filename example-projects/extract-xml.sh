#!/bin/bash
# Extract the XML Producer.Dat from every Movie Maker file that can
# be found.
# Small disclaimer: Qt's XML parser swaps around the order of
# attributes when serializing the DOM again, and it leaves out
# whitespace, so the output of the xml command is not *exactly*
# what is contained in the MSWMM file.

toolPath="../build/mswmm-tool"
if [ ! -f  "$toolPath" ]; then
  echo "Please build the project first."
  exit 1
fi


IFS=$'\n' files=$(find . -name "*.MSWMM")
for i in ${files[@]}
do
  xmlFile=$(echo $i | sed "s/\.MSWMM$/\.xml/")
  ../build/mswmm-tool xml "$i" > "$xmlFile"
done
