/*!
 * Polyphonic synthesizer for microcontrollers.  Debug helper.
 * (C) 2016 Stuart Longland
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */
#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef _DEBUG
#include <stdio.h>
#define _DPRINTF(s, a...)	printf(__FILE__ ": %d " s, __LINE__, a)
#else
#define _DPRINTF(s, a...)
#endif

#endif
/*
 * vim: set sw=8 ts=8 noet si tw=72
 */
