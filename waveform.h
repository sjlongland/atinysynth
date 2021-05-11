/*!
 * Polyphonic synthesizer for microcontrollers.  Voice waveform generator.
 * (C) 2017 Stuart Longland
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
#ifndef _WAVEFORM_H
#define _WAVEFORM_H
#include <stdint.h>

/*!
 * Waveform generator state.  12 bytes.
 */
struct voice_wf_gen_t {
	/*! Waveform output sample in fixed-point */
	int16_t sample;
	/*! Amplitude sample in fixed point */
	int16_t amplitude;
	/*! Samples to next waveform period (12.4 fixed point) */
	uint16_t period_remain;
	/*!
	 * Period duration in samples (12.4 fixed point).
	 * (Half period for SQUARE and TRIANGLE)
	 */
	uint16_t period;
	/*! Amplitude step for TRIANGLE and SAWTOOTH */
	int16_t step;
	uint8_t reserved;
	/*! Waveform generation mode */
	uint8_t mode;
};

/* Waveform generation modes */
#define VOICE_MODE_DC		(0)
#define	VOICE_MODE_SQUARE	(1)
#define VOICE_MODE_SAWTOOTH	(2)
#define VOICE_MODE_TRIANGLE	(3)
#define VOICE_MODE_NOISE	(4)

/**
 * The Waveform definition. 4 bytes.
 */ 
struct voice_wf_def_t {
	/*! Waveform generation mode, see VOICE_MODE_ enumerated values */
	uint8_t mode;
	/*! Waveform amplitude */
	int8_t amplitude;
	/*! Waveform full period as `sample_freq / frequency` (if applicable) */
	uint16_t period;
};

/* Compute frequency period of a generic wave */
uint16_t voice_wf_freq_to_period(uint16_t frequency);

/*!
 * Configure the generator for a DC offset synthesis.
 */
void voice_wf_set_dc(struct voice_wf_gen_t* const wf_gen,
		int8_t amplitude);

/*!
 * Configure the generator for square wave synthesis.
 */
void voice_wf_set_square(struct voice_wf_gen_t* const wf_gen,
		uint16_t freq, int8_t amplitude);

/*!
 * Configure the generator for sawtooth wave synthesis.
 */
void voice_wf_set_sawtooth(struct voice_wf_gen_t* const wf_gen,
		uint16_t freq, int8_t amplitude);

/*!
 * Configure the generator for sawtooth wave synthesis.
 */
void voice_wf_set_triangle(struct voice_wf_gen_t* const wf_gen,
		uint16_t freq, int8_t amplitude);

/*!
 * Configure the generator for pseudorandom noise synthesis.
 */
void voice_wf_set_noise(struct voice_wf_gen_t* const wf_gen,
		int8_t amplitude);

/*!
 * Configure the generator using waveform type and common parameters
 */
void voice_wf_set(struct voice_wf_gen_t* const wf_gen, struct voice_wf_def_t* const wf_def);

/*!
 * Retrieve the next sample from the generator.
 */
int8_t voice_wf_next(struct voice_wf_gen_t* const wf_gen);

#endif
/*
 * vim: set sw=8 ts=8 noet si tw=72
 */
