/*!
 * Polyphonic synthesizer for microcontrollers.
 * (C) 2021 Luciano Martorella
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
#ifndef _SEQUENCER_H
#define _SEQUENCER_H

#include "adsr.h"
#include "waveform.h"
#include "synth.h"

/*! 
 * Define a single step/frame of the sequencer. It applies to the active channel.
 * Contains the definition of the next waveform and envelope.
 * 16 bytes
 */
struct seq_frame_t {
	/*! Envelope definition */
	struct adsr_env_def_t adsr_def;
	/*! Waveform definition */
	struct voice_wf_def_t waveform_def;
};

struct seq_stream_header_t {
	/*! Sampling frequency required for correct timing */
	uint16_t synth_frequency;
	/*! Number of voices. They will all be enabled */
	uint8_t voices;
	/*! Total frame count */
	uint16_t frames;
	/*! Follow frames data, as stream of seq_frame_t */
};

/*! 
 * Plays a stream sequence of frames, in the order requested by the synth.
 * The frames must then be sorted in the same fetch order and not in channel order.
 * Frames will be fed using the handler passed by `seq_set_stream_require_handler`.
 */
int seq_play_stream(const struct seq_stream_header_t* stream_header, uint8_t voice_count, struct poly_synth_t* synth);

/*! Requires a new frame. The handler must return 1 if a new frame was acquired, or zero if EOF */
void seq_set_stream_require_handler(uint8_t (*handler)(struct seq_frame_t* frame));

/*! Use it when `seq_play_stream` is in use, must be called at every sample */
void seq_feed_synth(struct poly_synth_t* synth);

/*! List of frames, used by `seq_frame_map_t` */
struct seq_frame_list_t {
	/*! Frame count */
	int count;
	/*! List of frames */
	struct seq_frame_t* frames;
};

/*! 
 * A frame map is an intermediate data in which frames are organized by channel.
 * Typically requires dynamic memory allocation (heap) to allocate the whole tune
 * upfront.
 */
struct seq_frame_map_t {
	/*! Channel count */
	int channel_count;
	/*! Array of frame lists */
	struct seq_frame_list_t* channels;
}; 

/*! Compile/reorder a frame-map (by channel) to a sequential stream */
void seq_compile(struct seq_frame_map_t* map, struct seq_frame_t** frame_stream, int* frame_count, int* voice_count);

/*! Free the stream allocated by `seq_compile`. */
void seq_free(struct seq_frame_t* frame_stream);

#endif
