#!/bin/sh

echo "Porta library link setup, v28Sep2004_1412"
echo "run from the project directory which uses porta"
echo "and pass the porta files used by it, as cmdline args to me"

portaPath=/hanishkvc/work/kvctek/projs/porta

for curFile in $@; do

createLink=0
diff $portaPath/$curFile $curFile
if [ $? -ne 0 ]; then
  read -p "[$curFile] differs, press any key to see difference"
  diff $portaPath/$curFile $curFile | less
  read -p "remove [$curFile] file and link to porta base,[yn]" uInput
  echo "You typed [$uInput]"
  if [ $uInput == "y" ]; then
    createLink=1;
  fi 
else
  echo "[$curFile] same, so linking to porta base"
  createLink=1;
fi

if [ $createLink -eq 1 ]; then
  echo "[$curFile] linking to porta base"
  rm $curFile;
  ln -s $portaPath/$curFile $curFile
else
  echo "[$curFile] left as it is"
fi

done
