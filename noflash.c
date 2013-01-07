// $Id$
//
// noflash.c
//
// This is 'noflash', a simple program to start a Win32 console-mode
// application without opening a console window. It uses CreateProcess
// to fork off the console mode app, just like recipe 15.17 in
// The Perl Cookbook. In case you hadn't guessed, the author is=20
// Just Another Perl Hacker needing to run Perl scripts unobtrusively.
//
// (c) 1999 Tripp Lilley, Innovative Workflow Engineering, Inc.
//          tripp.lilley@iweinc.com
//
//
// noflash is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.

// noflash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You probably did not receive a copy of the GNU General Public License
// along with noflash, because the license is larger than the code!
// You can find it at <http://www.gnu.org/copyleft/gpl.html>. Though
// I hardly think you will, you may write to the FSF at:
//
// Free Software Foundation, Inc.
// 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// I encourage you to support Free Software by buying meals and
// stiff drinks for the authors :-)

#include <windows.h>
#include <stdio.h>

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
		    LPSTR lpszCmdParam, int nCmdShow)
{
  BOOL result;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  si.cb              = sizeof(STARTUPINFO);
  si.lpReserved      = NULL;
  si.lpReserved2     = NULL;
  si.cbReserved2     = STARTF_USESHOWWINDOW;
  si.lpDesktop       = NULL;
  si.lpTitle         = NULL;
  si.dwFlags         = 0;
  si.dwX             = 0;
  si.dwY             = 0;
  si.dwFillAttribute = 0;
  si.wShowWindow     = SW_HIDE;

  result = CreateProcess( NULL,       // image name
			  lpszCmdParam,                      // command line
			  NULL,                              // process security attributes
			  NULL,                              // thread security attributes
			  FALSE,                             // inherit handles?
			  DETACHED_PROCESS,                  // type of process to create
			  NULL,                              // environment block? = (NULL=inherit)
			  NULL,                              // current working directory
			  &si,                               // startup info
			  &pi );                             // process information

  if( !result ) return( 1 );
  return
}
