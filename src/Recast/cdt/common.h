/*****************************************************************************
**
** common.h: an include file containing general type definitions, macros,
** and inline functions used in the other modules.
**
** Copyright (C) 1995 by Dani Lischinski 
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#include <cstdlib>
#include <cassert>

#ifdef NDEBUG
#	define Assert(e)	(void(0))
#	define Warning(msg)	(void(0))
#else
#	define Assert(e)	assert(e)
#	define Warning(msg) \
		(cerr << "Warning: " << msg \
		      << ", file " << __FILE__ \
		      << ", line " << __LINE__ << "\n")
#endif
#define Abort(msg) \
		((cerr << "Fatal Error: " << msg \
		       << ", file " << __FILE__ \
		       << ", line " << __LINE__ << "\n"), abort())
	
#endif
