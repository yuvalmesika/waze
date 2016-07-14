/* roadmap_native_keyboard.c - WIN keyboard specific functionality
 *
 * LICENSE:
 *   Copyright 2009, Waze Ltd
 *   Alex Agranovich
 *   Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *
 */
#include "../roadmap.h"
#include "../roadmap_config.h"
#include "../roadmap_native_keyboard.h"
#include <SIPAPI.H>


extern RoadMapConfigDescriptor RoadMapConfigUseNativeKeyboard;
static BOOL initialized = FALSE;

/***********************************************************
 *  Name        : roadmap_native_keyboard_enabled
 *  Purpose     : If the native keyboard enabled
 *	Params		: void
 *
 */
BOOL roadmap_native_keyboard_enabled( void )
{
 
#ifdef EMBEDDED_CE
	return FALSE;
#else
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigUseNativeKeyboard), "yes"))
      return TRUE;
   return FALSE;
#endif
}


/***********************************************************
 *  Name        : roadmap_native_keyboard_show_search
 *  Purpose     :
 *	Params		: params - set of parameters for the keyboard representation
 *
 */
void roadmap_native_keyboard_show( RMNativeKBParams* params )
{
#ifndef EMBEDDED_CE
	SipShowIM(SIPF_ON);
#endif
}

/***********************************************************
 *  Name        : roadmap_keyboard_hide
 *  Purpose     : Hide native keyboard
 *
 *
 */
void roadmap_native_keyboard_hide( void )
{
#ifndef EMBEDDED_CE
	SipShowIM(SIPF_OFF);
#endif
}

/***********************************************************
 *  Name        : roadmap_keyboard_hide
 *  Purpose     : Returns true if the native keyboard is currently visible
 *
 */
BOOL roadmap_native_keyboard_visible( void )
{
#ifndef EMBEDDED_CE
  SIPINFO si;

  memset (&si, 0, sizeof (si));
  si.cbSize = sizeof (si);
  if (SipGetInfo(&si))
  {
    return (si.fdwFlags && SIPF_ON);
  }
  else
	return FALSE;
#else
	return FALSE;
#endif
}

/***********************************************************
 *  Name        : roadmap_native_keyboard_get_params
 *  Purpose     : Returns params set ( bitwise copy )
 *
 */
void roadmap_native_keyboard_get_params( RMNativeKBParams* params_out )
{
}
