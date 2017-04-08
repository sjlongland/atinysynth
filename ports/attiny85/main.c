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

#include "synth.h"
#include "fifo.h"
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define SAMPLE_LEN	128
static volatile uint8_t sample_buffer[SAMPLE_LEN];
static struct fifo_t sample_fifo;
volatile uint8_t underflow = 0;

struct voice_ch_t poly_voice[16];
struct poly_synth_t synth;

int main(void) {
#if 0
	/* Turn clock speed right up */
	CLKPR = (1 << CLKPCE);

	/* Turn on all except ADC */
	PRR = (1 << PRADC);

	/* Start up PLL */
	PLLCSR = (1 << PLLE);
	while (!(PLLCSR & (1<<PLOCK)));
	PLLCSR |= (1<<PCKE);
#endif

#if 1
	fifo_init(&sample_fifo, sample_buffer, SAMPLE_LEN);
	memset(poly_voice, 0, sizeof(poly_voice));
	synth.voice = poly_voice;
	synth.enable = 0;
	synth.mute = 0;
#endif

	/* Enable output on PB4 (PWM out) */
	DDRB |= (1 << 4) | (1 << 3);
	PORTB &= ~((1 << 4) | (1 << 3));

	/* Timer 1 configuration for PWM */
	OCR1B = 128;			/* Initial PWM value */
	OCR1C = 255;			/* Maximum PWM value */
	TCCR1 = (1 << CS10);		/* No prescaling, max speed */
	GTCCR = (1 << PWM1B)		/* Enable PWM */
		| (2 << COM1B0);	/* Clear output bit on match */

	/* Timer 0 configuration for sample rate interrupt */
	TCCR0A = (2 << WGM01);		/* CTC mode */
	TCCR0B = (1 << CS01);		/* 1/8 prescaling */
	/* Sample rate */
	OCR0A = (uint8_t)((uint32_t)F_CPU / (8*(uint32_t)SYNTH_FREQ));
	TIMSK |= (1 << OCIE0A);		/* Enable interrupts */

#if 1
	/* Configure the synthesizer */
	synth.enable = 1;
	voice_wf_set_square(&poly_voice[0].wf, 1000, 127);
	poly_voice[0].adsr.time_scale = 32;
	poly_voice[0].adsr.sustain_time = 120;
	poly_voice[0].adsr.sustain_amp = 255;
#endif

#if 1
	sei();
	while(1) {
#if 1
		while (sample_fifo.stored_sz < SAMPLE_LEN) {
			int16_t s = poly_synth_next(&synth);
			fifo_write_one(&sample_fifo,
					128 + (s >> 9));
#if 1
			PORTB ^= (1 << 3);
#endif
			if (poly_voice[0].adsr.state ==
					ADSR_STATE_DONE)
				adsr_reset(&poly_voice[0].adsr);
			while(underflow);
		}

#if 0
		poly_evt.flags = POLY_EVT_TYPE_IFREQ;
		poly_evt.value += 100;
		if (poly_evt.value > 2000)
			poly_evt.value = 100;
		poly_load(&poly_evt);
#endif
#endif
	}
#else
	while (1) {
		int i;
		for (i = 0; i < 255; i++) {
			OCR1B = i;
			_delay_us(100);
		}
		for (i = 255; i > 0; i--) {
			OCR1B = i;
			_delay_us(100);
		}
		PORTB ^= (1 << 3);
	}
#endif
	return 0;
}

ISR(TIM0_COMPA_vect) {
#if 1
	uint8_t sample = fifo_read_one(&sample_fifo);
	if (sample >= 0) {
		OCR1B = sample;
	} else {
		OCR1B = 128;
		underflow = 1;
	}
#else
	PORTB ^= (1 << 3);
#endif
}
