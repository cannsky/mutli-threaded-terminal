#Variables
value=$1
tempValue=$1
div1=0
div2=0
number=0
sum=0
#while the remainded value is greater than 1, continue
while [ $value -ge 1 ]
do
	#set temp value so that we can calculate two digits
	tempValue=$value
	div1=`expr $tempValue % 10`
	tempValue=`expr $tempValue / 10`
	#if we are at the last digit don't calculate
	if [ $tempValue -ge 1 ]
	then
		#calculations
		div2=`expr $tempValue % 10`
		number=`expr $div1 \* 10`
		number=`expr $number + $div2`
		sum=`expr $number + $sum`
	fi
	value=$tempValue
done
#print sum
echo "$sum"
#exit with success
exit 0
