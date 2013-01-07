/*
Copyright (C) 1998 Evgenii Rudnyi, rudnyi@comp.chem.msu.su
                                   http://www.chem.msu.su/~rudnyi/

This software is free; you can redistribute it and/or modify it under the 
terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.  This software is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.
http://www.gnu.org/copyleft/gpl.htm
*/

#include <windows.h>
#include <iostream.h>

const char *HELP =
"\nEvgenii Rudnyi (C) All rights reserved, 1998 \n\
\tChemistry Department, Moscow State University \n\
\t119899 Moscow, Russia, http://www.chem.msu.su/~rudnyi/welcome.html \n\
\trudnyi@comp.chem.msu.su \n\
This utility is freeware and in public domain. Feel free to use and \n\
distribute it. Optionally, provided you like the utility, \n\
you may send me a bottle of beer. \n\n\
Disclaimer of warranty: \n\
This utility is supplied as is. I disclaim all warranties, \n\
express or implied, including, without limitation, the warranties of \n\
merchantability and of fitness of this utility for any purpose. I assume \n\
no liability for damages direct or consequential, which may result from \n\
the use of this utility. \n\n\
The goal of the utility is to obtain the account name from SID, usage:\n\
\tsid2user [\\\\computer_name] authority subauthority_1 ... \n\
where computer_name is optional. For example, \n\
\tsid2user 5 32 544 \n\
By default, the search starts at a local Windows NT computer. \n";

int main(int argc, char *argv[])
{
  if (argc == 1)
  {
    cout << HELP;
    return 0;
  }
  LPCTSTR lpSystemName = NULL;     // address of string for system name
  int start = 1;
  if (argv[1][0]=='\\' && argv[1][1]=='\\')
  {
    if (argc == 2)
    {
      cout << HELP << endl;
      return 0;
    }
    lpSystemName = argv[1] + 2;
    start = 2;
  }
  SID_IDENTIFIER_AUTHORITY sia = {0,0,0,0,0,atoi(argv[start])};
  int n = argc - start - 1;
  int s[8];
  for (int i = 0; i < n; ++i)
  s[i] = atoi(argv[i+start+1]);
  SID_NAME_USE snu;
  PSID pSid;
  UCHAR buffer1[1024];
  UCHAR buffer2[1024];
  UCHAR buffer3[1024];
  DWORD length = 900;
  if (!AllocateAndInitializeSid(&sia,
                                n,
                                s[0],
                                s[1],
                                s[2], s[3], s[4],
                                s[5], s[6], s[7],
                                &pSid))
  {
    cout << endl << "could not allocate SID" << endl;
    exit(1);
  }
  if(LookupAccountSid(lpSystemName,
                      pSid,
                      buffer1,
                      &length,
                      buffer2,
                      &length,
                      &snu))
  {
    cout << endl;
    CharToOem(buffer1, buffer3);
    cout << "Name is " << buffer3 << endl;
    CharToOem(buffer2, buffer3);
    cout << "Domain is " << buffer3 << endl;
    cout << "Type of SID is ";
    switch (snu)
    {
      case SidTypeUser:
        cout << "SidTypeUser" << endl;
        break;
      case SidTypeGroup:
        cout << "SidTypeGroup" << endl;
        break;
      case SidTypeDomain:
        cout << "SidTypeDomain" << endl;
        break;
      case SidTypeAlias:
        cout << "SidTypeAlias" << endl;
        break;
      case SidTypeWellKnownGroup:
        cout << "SidTypeWellKnownGroup" << endl;
        break;
      case SidTypeDeletedAccount:
        cout << "SidTypeDeletedAccount" << endl;
        break;
      case SidTypeInvalid:
        cout << "SidTypeInvalid" << endl;
        break;
      default:
        cout << "SidTypeUnknown" << endl;
    }
  }
  else
    cout << endl << "LookupSidName failed - no such account" << endl;
  FreeSid(pSid);
}


