#!/bin/bash
files=`find ./../../../data/data/menu/ -name '*.xml'`
for item in $files
do
  echo 'Processing ' $item
  xmllint --format $item > test.tmp && mv test.tmp $item
done

