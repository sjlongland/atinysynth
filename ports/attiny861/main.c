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
#define VOICES		(8)

/*! Voice states */
struct voice_ch_t poly_voice[VOICES];

/*! Synthesizer state */
struct poly_synth_t synth;

/*! 1-millisecond timer tick */
static volatile uint16_t ms_timer = 0;

/*! Amplifier turn-off delay in seconds */
static volatile uint16_t amp_powerdown = 0;

/*!
 * Delay in powering down amplifier, since it makes an annoying pop when
 * it does power down.  (Sample ticks)
 */
#define AMP_POWERDOWN_DELAY	(60000)

/*! Number of I/O channels */
#define CHANNELS	(8)

/*!
 * Voice definitions for all the channels.
 */
const struct adsr_env_def_t voice_def = {
	.time_scale = 100,
	.delay_time = 0,
	.attack_time = 10,
	.decay_time = 10,
	.sustain_time = ADSR_INFINITE,
	.release_time = 10,
	.peak_amp = 255,
	.sustain_amp = 192
};

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
 * Button states, current, last, hit flags and release flags… and which ones
 * we saw on power-up.
 */
static volatile uint8_t
	button_state = 0,
	button_enable = 0;

/*!
 * LED amplitudes… 0 = off
 */
static uint8_t light_output[CHANNELS] = {0, 0, 0, 0, 0, 0, 0, 0};


/*!
 * Trigger playback of a tone for a button.
 *
 * @param	b	Button ID number (0…CHANNELS-1)
 */
static void trigger_button(uint8_t b) {
	struct voice_ch_t* voice = &poly_voice[b];

	if (voice) {
		uint16_t freq = pgm_read_dword(&button_freq[b]);
		adsr_config(&voice->adsr, &voice_def);
		voice_wf_set_triangle(&voice->wf, freq, 127);

		synth.enable |= (1 << b);
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

	/* Determine which buttons are in use: they are pulled high. */
	_delay_ms(100);
	button_enable = PINA;

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

	/* Turn on interrupts */
	sei();

	/* Turn on amp early */
	PORTB |= AUDIO_EN;
	amp_powerdown = AMP_POWERDOWN_DELAY;

	/* Enter main loop */
	while(1) {
		/* Count down millisecond timer */
		if (!ms_timer) {
			/* One millisecond has passed */
			ms_timer = SYNTH_FREQ/10;

			/* Tick down the amplifier power-down timer */
			if ((!synth.enable) && amp_powerdown)
				amp_powerdown--;
		}

		/* Check the button states */
		uint8_t b = 0;
		uint8_t bm = 1;
		for (b = 0, bm = 1; b < CHANNELS; b++, bm <<= 1) {
			struct voice_ch_t* voice = &poly_voice[b];

			if (adsr_is_idle(&voice->adsr)) {
				/* Has the button been pressed? */
				if (button_state & bm) {
					trigger_button(b);
				} else {
					light_output[b] = 0;
				}
			} else if (adsr_is_done(&voice->adsr)) {
				adsr_reset(&voice->adsr);
			} else {
				/* See if it is time to release? */
				if (adsr_is_waiting(&voice->adsr)
						&& (~button_state & bm)) {
					adsr_continue(&voice->adsr);
				}

				/* Update the LED for that channel */
				light_output[b] = voice->adsr.amplitude;
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

#define GPIO_STATE_READ		(0)
#define GPIO_STATE_SELECT	(1)
#define GPIO_STATE_PWM		(2)

ISR(TIMER0_COMPA_vect) {
	static uint8_t gpio_state = GPIO_STATE_READ;
	static uint8_t cur_light = 0;

	switch (gpio_state) {
	case GPIO_STATE_READ:
		{
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
				button_state = now & button_enable;
			}
			/* Turn off lights */
			OCR1D = 0;
			/* Switch to setting outputs */
			PORTB &= ~GPIO_EN;
			DDRA = 0xff;
			gpio_state = GPIO_STATE_SELECT;
			break;
		}
	case GPIO_STATE_SELECT:
		/* We are setting outputs */
		PORTA = (1 << cur_light);
		/* Switch on input MUXes */
		PORTB |= GPIO_EN;
		/* Switch to PWM */
		PORTA = 0xff;
		DDRA = 0x00;
		gpio_state = GPIO_STATE_PWM;
		break;
	case GPIO_STATE_PWM:
		OCR1D = light_output[cur_light];
		/* Increment the light for later (modulo 8) */
		cur_light++;
		cur_light &= 0x07;
		gpio_state = GPIO_STATE_READ;
		break;
	}

	/* Tick down the one-second timer */
	if (ms_timer)
		ms_timer--;

	/* Compute and output the next sample */
	int8_t s = poly_synth_next(&synth);
	OCR1B = s + 128;
}
