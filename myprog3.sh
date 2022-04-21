#!/bin/bash
#grep command will search for the string given in $2
a=$(grep -o $2 $1 | wc -l)

#sed command will replace the string in $2 with $3
if [[ $2 != "" && $3 != "" ]]; then
	sed -i "s/$2/$3/g" $1
fi
#output
echo "All $a occurrences of “$2” in “$1” has changed with “$3”"