#define LITTLE_ENDIAN 0
#define BIG_ENDIAN    1

int machineEndianness()
{
   int i = 1;
   char *p = (char *) &i;
   if (p[0] == 1) // Lowest address contains the least significant byte
      return LITTLE_ENDIAN;
   else
      return BIG_ENDIAN;
}
