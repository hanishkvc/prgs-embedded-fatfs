#include <stdio.h>


int iCur=0x80000000, niCur;
unsigned int uCur;

int main()
{
  uCur = (unsigned int)iCur;
  printf("int [0x%x:%d] unsigned [0x%x:%u]\n",iCur,iCur,uCur,uCur);
  niCur = (int)uCur;
  printf("int [0x%x:%d] unsigned [0x%x:%u]\n",niCur,niCur,uCur,uCur);
  return 0;
}

