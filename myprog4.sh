shopt -s nullglob
#Values
arr=($PWD/*)

maxSize=0
minSize=10000

maxFileToReplaced=""
minFileToReplaced=""
#Check each file
for f in "${arr[@]}"; do
	#if directory pass
	if [[ -d $f ]]; 
	then
		echo "skipping directory"
	else
		#check the size of the file
		#if size is better choose this file
		val=`wc -c < "$f"`
		if [ $maxSize -le $val ]
		then
			if [ $f != "$PWD/`basename $0`" ]
			then
				maxFileToReplaced=$f
				maxSize=$val
			fi
		fi
	fi
done
#create directory for largest if doesn't exist
mkdir -p "$PWD/largest/"
#move largest file to the directory
mv $maxFileToReplaced "$PWD/largest/`basename $maxFileToReplaced`"

arr2=($PWD/*)

for f in "${arr2[@]}"; do
	if [[ -d $f ]]; then
		echo "skipping directory"
	else
		val=`wc -c < "$f"`
		if [ $minSize -ge $val ]
		then
			if [ $f != "$PWD/`basename $0`" ]
			then
				minFileToReplaced=$f
				minSize=$val
			fi
		fi
	fi
done

mkdir -p "$PWD/smallest/"
mv $minFileToReplaced "$PWD/smallest/`basename $minFileToReplaced`"

echo "$maxFileToReplaced"
echo "$minFileToReplaced"
