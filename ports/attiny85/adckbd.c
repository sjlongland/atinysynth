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

#include <avr/io.h>
#include <avr/interrupt.h>
#include "adckbd.h"

/*!
 * Last known keyboard state
 */
uint8_t adckbd_last = 0;

/*!
 * Current keyboard state
 */
uint8_t adckbd_now = 0;

/*!
 * Keyboard state differences from last
 */
uint8_t adckbd_diff = 0;

/* Keyboard thresholds */
#define B0_THRESHOLD	0xe8	/*!< Keyboard button 1 threshold */
#define B1_THRESHOLD	0xc2	/*!< Keyboard button 2 threshold */
#define B2_THRESHOLD	0xa7	/*!< Keyboard button 3 threshold */
#define B3_THRESHOLD	0x80	/*!< Keyboard button 4 threshold */

/*!
 * Initialise the ADC keyboard.
 */
void adckbd_init() {
	/* This code generates an "operand out of range" assembler error */
#if 0
	/* Turn on ADC */
	PRR &= ~(1 << PRADC);
#endif

	/* Select ADC3, 5V reference, left-justify output */
	ADMUX = (3 << MUX0) | (1 << ADLAR);

	/* Free running mode */
	ADCSRB = 0;

	/* Minimum speed ADC, Enable ADC, with interrupts, auto-trigger */
	ADCSRA = (1 << ADPS2)
		| (1 << ADPS1)
		| (1 << ADPS0)
		| (1 << ADEN)
		| (1 << ADATE)
		| (1 << ADIE)
		| (1 << ADSC)
	;
}

ISR(ADC_vect) {
	uint8_t adc = ADCH;
	adckbd_last = adckbd_now;

	if (adc > B0_THRESHOLD)
		adckbd_now |= ADCKBD_PRESSED(0);
	else if (adc > B1_THRESHOLD)
		adckbd_now |= ADCKBD_PRESSED(1);
	else if (adc > B2_THRESHOLD)
		adckbd_now |= ADCKBD_PRESSED(2);
	else if (adc > B3_THRESHOLD)
		adckbd_now |= ADCKBD_PRESSED(3);
	else
		adckbd_now |= ADCKBD_RELEASED;

	adckbd_diff = (adckbd_now & ADCKBD_ALL)
		^ (adckbd_last & ADCKBD_ALL);

	if (adckbd_diff)
		adckbd_now |= ADCKBD_CHANGED;
}
