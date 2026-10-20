/***************************************************************************

    file        : racestate.h
    created     : Sat Nov 16 14:05:06 CET 2002
    copyright   : (C) 2002 by Eric Espie
    email       : eric.espie@torcs.org

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file

    @author	<a href=mailto:torcs@free.fr>Eric Espie</a>
*/

#ifndef _RACESTATE_H_
#define _RACESTATE_H_

extern void ReStateInit(void *prevMenu);
extern void ReStateManage(unsigned ms = 0);
extern void ReStateApply(int state);

#endif /* _RACESTATE_H_ */
