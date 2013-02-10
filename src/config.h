/*
 * Copyright (C) 2013 Hans Oesterholt <debian@oesterholt.net>
 * 
 * spod2html is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * spod2html is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CONFIG__H__
#define __CONFIG__H__
#ifndef __OS__CONFIG__H
#define __OS__CONFIG__H "$Id: Linux.h,v 1.1 2004/07/19 07:10:39 cvs Exp $"

#undef  HAVE_STRICMP
#define HAVE_STRCASECMP

#endif

#define PTHREAD_LIB ""
#define BIN_EXT ""
#define SHL_EXT ".so"
#define LIB_EXT ".a"
#define OBJ_EXT ".o"
#define VERSION "1.0.0"
#define PACKAGE "spodcxx"

#endif
