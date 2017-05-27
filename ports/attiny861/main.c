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

#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/* Pin allocations: port B */
#define SDA		(1 << 0) /*! I²C Serial Data		[I/O] */
#define MISO		(1 << 1) /*! SPI Master In Slave Out	[N/C] */
#define SCL		(1 << 2) /*! I²C Serial Clock		[I/O] */
#define AUDIO_PWM	(1 << 3) /*! Audio PWM			[O] */
#define AUDIO_EN	(1 << 4) /*! Audio Enable		[O] */
#define LIGHT_PWM	(1 << 5) /*! Light PWM (inverted)	[O] */
#define GPIO_EN		(1 << 6) /*! GPIO Enable		[O] */

struct voice_ch_t poly_voice[16];
struct poly_synth_t synth;

/*!
 * Button states, current, last, hit flags and release flags.
 */
static volatile uint8_t
	button_state = 0,
	button_last = 0,
	button_hit = 0,
	button_release = 0;

/*!
 * LED amplitudes
 */
static uint8_t light_output[8];

int main(void) {
	/* Initialise configuration */
	memset(poly_voice, 0, sizeof(poly_voice));
	synth.voice = poly_voice;
	synth.enable = 0;
	synth.mute = 0;

	/* Clear outputs. */
	PORTB = 0;
	DDRB |= (AUDIO_PWM|LIGHT_PWM|GPIO_EN|AUDIO_EN);

	/* Timer 1 configuration for PWM */
	OCR1B = 128;			/* Initial PWM value: audio */
	OCR1D = 32;			/* Initial PWM value: light */
	OCR1C = 255;			/* Maximum PWM value */
	TCCR1A = (2 << COM1B0)		/* Clear OC1B on match,
					   nOC1B not used */
		| (1 << PWM1B);		/* Enable PWM channel B */
	TCCR1B = (1 << CS10);		/* No prescaling, max speed */
	TCCR1C = (3 << COM1D0)		/* Set OC1D on match,
					   nOC1D not used */
		| (1 << PWM1D);		/* Enable PWM channel D */
	TCCR1D = (0 << WGM10);		/* Fast PWM mode */

	/* Timer 0 configuration for sample rate interrupt */
	TCCR0A = (1 << CTC0);		/* CTC mode */
	TCCR0B = (2 << CS00);		/* 1/8 prescaling */

	/* Sample rate */
	OCR0A = (uint8_t)((uint32_t)F_CPU / (8*(uint32_t)SYNTH_FREQ));
	TIMSK |= (1 << OCIE0A);		/* Enable interrupts */

	/* Configure the synthesizer */
	voice_wf_set_triangle(&poly_voice[0].wf, 523, 63);
	voice_wf_set_triangle(&poly_voice[1].wf, 659, 63);
	voice_wf_set_triangle(&poly_voice[2].wf, 784, 63);
	voice_wf_set_triangle(&poly_voice[3].wf, 880, 63);
	adsr_config(&poly_voice[0].adsr,
			100, 0, 10, 10, 10, 10, 255, 192);
	adsr_config(&poly_voice[1].adsr,
			100, 0, 10, 10, 10, 10, 255, 192);
	adsr_config(&poly_voice[2].adsr,
			100, 0, 10, 10, 10, 10, 255, 192);
	adsr_config(&poly_voice[3].adsr,
			100, 0, 10, 10, 10, 10, 255, 192);
	synth.enable = 0x0;

	sei();
	while(1) {
		PORTB ^= (1 << 0);
#if 0
		if (adckbd_now & ADCKBD_CHANGED) {
			uint8_t btn = 0;
			uint8_t voice = 0;
			for (btn = ADCKBD_PRESSED(0), voice = 0;
					btn & ADCKBD_ALL;
					btn <<= 1, voice++) {
				PORTB ^= (1 << 2);
				/* Is it pressed now? */
				if (!(adckbd_now & btn))
					continue;

				/* Was it pressed before? */
				if (!(adckbd_diff & btn))
					continue;

				adsr_reset(&poly_voice[voice].adsr);
				synth.enable |= btn;

				/* Acknowledge key */
				adckbd_now &= ~btn;
			}

			/* Acknowledge changes */
			adckbd_now &= ~ADCKBD_CHANGED;
			PORTB ^= (1 << 1);
		}
#endif
	}
	return 0;
}

ISR(TIMER0_COMPA_vect) {
	/* Internal state */
	static uint8_t cur_light = 0;	/*!< Currently enabled light */

	if (PORTB & GPIO_EN) {
		/* We are reading inputs */
		uint8_t diff;
		button_last = button_state;
		button_state = ~PINA;
		/* Determine differences from last */
		diff = button_state ^ button_last;
		/* Detect button presses & releases */
		button_hit |= button_state & diff;
		button_release |= (~button_state) & diff;
		/* Switch to setting outputs */
		PORTB &= ~GPIO_EN;
		DDRA = 0xff;
	} else {
		/* We are setting outputs */
		OCR1D = light_output[cur_light];
		PORTA = (1 << cur_light);
		/* Increment the light for later (modulo 8) */
		cur_light++;
		cur_light &= 0x07;
		/* Switch on input MUXes */
		PORTB |= GPIO_EN;
		/* Wait for latch */
		_delay_us(1);
		/* Switch to reading inputs */
		DDRA = 0x00;
	}

	/* Compute and output the next sample */
	int8_t s = poly_synth_next(&synth);
	OCR1B = s + 128;
}
