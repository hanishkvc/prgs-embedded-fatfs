#!/bin/sh

baseFile=rand4.log
size=`stat -c %s $baseFile`
for i in `ls *.fatucseek | cut -f 1 -d .`; do
 ((remSize=$size-$i))
 echo $i - $remSize;
 tail -c $remSize $baseFile > $i.temp
 diff $i.fatucseek $i.temp
 if [ $? != 0 ]; then
  echo $i.fatucseek FAILED;
 else
  echo $i.fatucseek SUCCESS;
 fi
done

