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
#include <avr/pgmspace.h>

/* Pin allocations: port B */
#define SDA		(1 << 0) /*! I²C Serial Data		[I/O] */
#define MISO		(1 << 1) /*! SPI Master In Slave Out	[N/C] */
#define SCL		(1 << 2) /*! I²C Serial Clock		[I/O] */
#define AUDIO_PWM	(1 << 3) /*! Audio PWM			[O] */
#define AUDIO_EN	(1 << 4) /*! Audio Enable		[O] */
#define LIGHT_PWM	(1 << 5) /*! Light PWM (inverted)	[O] */
#define GPIO_EN		(1 << 6) /*! GPIO Enable		[O] */

/*! Button debounce delay in sample rate ticks. */
#define DEBOUNCE_DELAY	(10)

/*! Number of voices */
#define VOICES		(16)

/*! Voice states */
struct voice_ch_t poly_voice[VOICES];

/*! Synthesizer state */
struct poly_synth_t synth;

/*! Amplifier turn-off delay */
static volatile uint8_t amp_powerdown = 0;

/*!
 * Delay in powering down amplifier, since it makes an annoying pop when
 * it does power down.  (Sample ticks)
 */
#define AMP_POWERDOWN_DELAY	(100)

/*! Number of I/O channels */
#define CHANNELS	(8)

/*!
 * I/O channel frequencies, note frequencies
 * from http://www.phy.mtu.edu/~suits/notefreqs.html
 */
const uint16_t button_freq[CHANNELS] PROGMEM = {
	262,	/* ~ Middle C (261.63 Hz) */
	294,	/* ~ Middle D (293.66 Hz) */
	330,	/* ~ Middle E (329.63 Hz) */
	349,	/* ~ Middle F (349.23 Hz) */
	392,	/* ~ Middle G (392.00 Hz) */
	440,	/* ~ Middle A (440.00 Hz) */
	494,	/* ~ Middle B (493.88 Hz) */
	523,	/* C5 (523.25 Hz) */
};

/*!
 * Button states, current, last, hit flags and release flags.
 */
static volatile uint8_t
	button_state = 0,
	button_last = 0,
	button_hit = 0,
	button_release = 0;

/*!
 * LED amplitudes… 0 = off
 */
static uint8_t light_output[CHANNELS] = {0, 0, 0, 0, 0, 0, 0, 0};

/*!
 * Button-voice assignments… -1 means no assignment
 */
static uint8_t button_voice[CHANNELS] = {-1, -1, -1, -1, -1, -1, -1, -1};

/*!
 * Find the voice channel number assigned to a button, or return NULL if
 * no channel is assigned.
 *
 * @param	b	Button ID number (0…CHANNELS-1)
 * @returns		Associated voice channel
 * @retval	NULL	Not assigned a voice channel
 */
static struct voice_ch_t* get_button_voice(uint8_t b) {
	uint8_t v = button_voice[b];

	if (v == -1)
		return NULL;	/* Not assigned */

	if (!(synth.enable & (1 << v))) {
		/* Stale assignment */
		button_voice[b] = -1;
		return NULL;
	}

	return &poly_voice[v];
}


/*!
 * Trigger playback of a tone for a button.
 *
 * @param	b	Button ID number (0…CHANNELS-1)
 */
static void trigger_button(uint8_t b) {
	/* Find a free voice channel */
	uint8_t v = 0, vm = 0;
	struct voice_ch_t* voice;
	for (v = 0, vm = 1;
			v < VOICES;
			v++, vm <<= 1) {
		if (!(synth.enable & vm)) {
			voice = &poly_voice[v];
			button_voice[b] = v;
			break;
		}
	}

	if (voice) {
		uint16_t freq = pgm_read_dword(&button_freq[b]);
		adsr_config(&voice->adsr,
				100, 0, 10, 10,
				ADSR_INFINITE, 10, 255, 192);
		voice_wf_set_triangle(&voice->wf, freq, 63);

		synth.enable |= vm;
	}
}


int main(void) {
	/* Initialise configuration */
	memset(poly_voice, 0, sizeof(poly_voice));
	synth.voice = poly_voice;
	synth.enable = 0;
	synth.mute = 0;

	/* Clear outputs. */
	PORTB = 0;
	DDRB |= (AUDIO_PWM|LIGHT_PWM|GPIO_EN|AUDIO_EN);
	DDRA = 0xff;
	PORTA = 0;
	PORTB |= GPIO_EN;
	_delay_ms(1);
	DDRA = 0x00;
	PORTA = 0;

	/* Timer 1 configuration for PWM */
	OCR1C = 255;			/* Maximum PWM value */
	TC1H = 0;			/* Reset counter high bits */
	TCNT1 = 0;			/* Reset counter low bits */
	OCR1A = 0;			/* Initial PWM value: spare */
	OCR1B = 127;			/* Initial PWM value: audio */
	OCR1D = 0;			/* Initial PWM value: light */
	TCCR1A = (1 << PWM1B);		/*
					 * Clear TCCR1A except for PWM1B,
					 * we'll configure OC1B
					 * via the shadow bits in TCCR1C
					 */
	TCCR1B = (1 << CS10);		/* No prescaling, max speed */
	TCCR1C = (2 << COM1B0S)		/* Clear OC1B on match,
					   nOC1B not used */
		| (3 << COM1D0)		/* Set OC1D on match,
					   nOC1D not used */
		| (1 << PWM1D);		/* Enable PWM channel D */
	TCCR1D = (0 << WGM10);		/* Fast PWM mode */
	TCCR1E = 0;			/* Not used: PWM6 mode */

	/* Timer 0 configuration for sample rate interrupt */
	TCCR0A = (1 << CTC0);		/* CTC mode */
	TCCR0B = (2 << CS00);		/* 1/8 prescaling */

	/* Sample rate */
	OCR0A = (uint8_t)((uint32_t)F_CPU / (8*(uint32_t)SYNTH_FREQ));
	TIMSK |= (1 << OCIE0A);		/* Enable interrupts */

	/* Turn off all channels */
	synth.enable = 0x0;
	synth.mute = 0x0;

	sei();
	while(1) {
		/* Check the button states */
		uint8_t b = 0;
		uint8_t bm = 1;
		for (b = 0, bm = 1; b < CHANNELS; b++, bm <<= 1) {
			struct voice_ch_t* voice = get_button_voice(b);

			if (voice) {
				/* See if it is time to release? */
				button_hit &= ~bm;
				if ((voice->adsr.state ==
						ADSR_STATE_SUSTAIN_EXPIRE)
						&& (button_release & bm)) {
					adsr_continue(&voice->adsr);
					button_release &= ~bm;
				}

				/* Update the LED for that channel */
				light_output[b] = voice->adsr.amplitude;
			} else {
				/* Has the button been pressed? */
				button_release &= ~bm;
				if (button_hit & bm) {
					button_hit &= ~bm;
					trigger_button(b);
				} else {
					light_output[b] = 0;
				}
			}
		}

		/* If there are channels enabled, turn on the amplifier */
		if (synth.enable) {
			PORTB |= AUDIO_EN;

			if (!amp_powerdown)
				amp_powerdown = AMP_POWERDOWN_DELAY;
		} else if (!amp_powerdown) {
			PORTB &= ~AUDIO_EN;
		}
	}
	return 0;
}

ISR(TIMER0_COMPA_vect) {
	if (PORTB & GPIO_EN) {
		/* We are reading inputs */
		static uint8_t last = 0;
		static uint8_t delay = DEBOUNCE_DELAY;
		uint8_t now = ~PINA;

		if (now != last) {
			/*
			 * We detected a change… reset
			 * debounce timer
			 */
			last = now;
			delay = DEBOUNCE_DELAY;
		} else if (delay) {
			/* Wait for bouncing to finish */
			delay--;
		} else {
			/* Bouncing finished */
			uint8_t diff;

			button_last = button_state;
			button_state = now;
			/* Determine differences from last */
			diff = button_state ^ button_last;
			/* Detect button presses & releases */
			button_hit |= button_state & diff;
			button_release |= (~button_state) & diff;
		}
		/* Switch to setting outputs */
		PORTB &= ~GPIO_EN;
		DDRA = 0xff;
	} else {
		/* We are setting outputs */
		static uint8_t cur_light = 0;
		OCR1D = light_output[cur_light];
		PORTA = (1 << cur_light);
		/* Increment the light for later (modulo 8) */
		cur_light++;
		cur_light &= 0x07;
		/* Switch on input MUXes */
		PORTB |= GPIO_EN;
		/* Switch to reading inputs */
		PORTA = 0;
		DDRA = 0x00;
	}

	/* Tick down the amplifier power-down timer */
	if ((!synth.enable) && amp_powerdown)
		amp_powerdown--;

	/* Compute and output the next sample */
	int8_t s = poly_synth_next(&synth);
	OCR1B = s + 128;
}
