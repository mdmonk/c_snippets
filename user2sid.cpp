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

#include <iostream.h>
#include <windows.h>

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
The goal of the utility is to obtain SID from the account name, usage:\n\
\tuser2sid [\\\\computer_name] account_name \n\
where computer_name is optional. By default, the search \n\
starts at a local Windows NT computer. \n";

int main(int argc, char *argv[])
{
  if (argc != 2 && argc != 3)
  {
    cout << HELP;
    return 0;
  }
  UCHAR buffer1[2048];
  UCHAR buffer2[2048];
  UCHAR buffer4[2048];
  UCHAR buffer3[4];
  DWORD length = 900;
  size_t sh=0;
  LPCTSTR lpSystemName;     // address of string for system name
  LPCTSTR lpAccountName;    // address of string for account name
  if (argc == 2)
  {
    lpSystemName = NULL;
    lpAccountName = argv[1];
  }
  else
  {
    if (argv[1][0]=='\\') ++sh;
    if (argv[1][1]=='\\') ++sh;
    lpSystemName = argv[1]+sh;
    lpAccountName = argv[2];
  }
  PSID Sid = buffer1;                 // address of security identifier
  LPDWORD cbSid = &length;            // address of size sid
  LPTSTR ReferencedDomainName = buffer2;      // address of string for referenced domain
  LPDWORD cbReferencedDomainName = &length;   // address of size domain string
  PSID_NAME_USE peUse = (PSID_NAME_USE)buffer3;         // address of structure for SID type

  if (LookupAccountName(lpSystemName, lpAccountName, Sid, cbSid,
                            ReferencedDomainName, cbReferencedDomainName,
                            peUse))
  {
    PSID_IDENTIFIER_AUTHORITY t = GetSidIdentifierAuthority(Sid);
    cout << endl << "S-1-";
    if (t->Value[0] == 0 && t->Value[1] == 0)
      cout <<  (ULONG)(t->Value[5]      )   +
               (ULONG)(t->Value[4] <<  8)   +
               (ULONG)(t->Value[3] << 16)   +
               (ULONG)(t->Value[2] << 24);
    else
      cout << hex << (USHORT)t->Value[0]
                  << (USHORT)t->Value[1]
                  << (USHORT)t->Value[2]
                  << (USHORT)t->Value[3]
                  << (USHORT)t->Value[4]
                  << (USHORT)t->Value[5]
           << dec;
    int n = *GetSidSubAuthorityCount(Sid);
    for (int i = 0; i < n; ++i)
    cout << '-' << *GetSidSubAuthority(Sid, i);
    cout << endl << endl;
    cout << "Number of subauthorities is " << n << endl;
    CharToOem(buffer2, buffer4);
    cout << "Domain is " << buffer4 << endl;
    cout << "Length of SID in memory is " << GetLengthSid(Sid) << " bytes" << endl;
    cout << "Type of SID is ";
    switch (*peUse)
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
    cout << endl << "LookupAccountName failed - no such account" << endl;
}
