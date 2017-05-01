/*!
 * Polyphonic synthesizer for microcontrollers: Atmel ATTiny85 port.
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

#include <stdint.h>

/*! Number of keys defined */
#define ADCKBD_NUM_KEYS		(4)

/*! Bit mask: key (n) pressed */
#define ADCKBD_PRESSED(n)	(1 << (n))

/*! Bit mask: all keys */
#define ADCKBD_ALL		((1 << (ADCKBD_NUM_KEYS)) - 1)

/*! Bit mask: keys released */
#define ADCKBD_RELEASED		(1 << 6)

/*! Bit mask: change detected */
#define ADCKBD_CHANGED		(1 << 7)

/*! Initialisation routine */
void adckbd_init();

/*!
 * Last known keyboard state
 */
extern uint8_t adckbd_last;

/*!
 * Current keyboard state
 */
extern uint8_t adckbd_now;

/*!
 * Differences from last
 */
extern uint8_t adckbd_diff;
