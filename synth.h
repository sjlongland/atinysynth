/*!
 * Polyphonic synthesizer for microcontrollers.
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
#ifndef _SYNTH_H
#define _SYNTH_H
#include "voice.h"
#include "debug.h"

/*
 * _POLY_CFG_H is a definition that can be given when compiling poly.c
 * and firmware code to define a header file that contains definitions for
 * the Polyphonic synthesizer.
 */
#ifdef SYNTH_CFG
#include SYNTH_CFG
#endif

#ifndef SYNTH_FREQ
/*!
 * Sample rate for the synthesizer: this needs to be declared in the
 * application.
 */
extern const uint16_t __attribute__((weak)) synth_freq;
#else
#define synth_freq		SYNTH_FREQ
#endif

/*!
 * Polyphonic synthesizer structure
 */
struct poly_synth_t {
	/*! Pointer to voices.  There may be up to 16 voices referenced. */
	struct voice_ch_t* voice;
	/*!
	 * Bit-field enabling given voices.  This allows selective turning
	 * on and off of given voice channels.  If the corresponding bit is
	 * not a 1, then that channel is not computed.
	 *
	 * Note no bounds checking is done, if you have only defined 4
	 * channels, then only set bits 0-3, don't set bits 4 onwards here.
	 */
	volatile uintptr_t enable;
	/*!
	 * Bit-field muting given voices.  This allows for selective adding
	 * of voices to the overall output.  If a field is 1, then that voice
	 * channel is not included.  (Note, disabled channels are *also*
	 * not included.)
	 */
	volatile uintptr_t mute;
};

/*!
 * Compute the next synthesizer sample.
 */
static inline int8_t poly_synth_next(struct poly_synth_t* const synth) {
	int16_t sample = 0;
	uintptr_t mask = 1;
	uint8_t idx = 0;

	while (mask) {
		if (synth->enable & mask) {
			/* Channel is enabled */
			int8_t ch_sample = voice_ch_next(
					&(synth->voice[idx]));
			_DPRINTF("poly %p ch=%d out=%d\n",
					synth, idx, ch_sample);
			if (!(synth->mute & mask))
				sample += ch_sample;
			if (voice_ch_is_done(&synth->voice[idx])) {
				_DPRINTF("poly %p ch=%d done\n", synth, idx);
				synth->enable &= ~mask;
				adsr_reset(&synth->voice[idx].adsr);
			}
		}
		idx++;
		mask <<= 1;
	}

	/* Handle clipping */
	if (sample > INT8_MAX)
		sample = INT8_MAX;
	else if (sample < INT8_MIN)
		sample = INT8_MIN;
	return sample;
};
#endif
/*
 * vim: set sw=8 ts=8 noet si tw=72
 */
