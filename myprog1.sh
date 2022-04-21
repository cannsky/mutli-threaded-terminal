#!/bin/bash
#while loop will read the input value and print '*' according to the value.
while IFS= read -r line || [[ -n "$line" ]]; do
	for (( i = $line; i > 0; i-- )); do
		echo -n "*"
	done
	echo ""
done < $1