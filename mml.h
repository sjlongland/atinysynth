/*!
 * Polyphonic synthesizer for microcontrollers.  MML parser.
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
#ifndef _MML_H
#define _MML_H

#include "voice.h"
#include "synth.h"
#include "sequencer.h"

/*! Manage parser errors, used to display it in pc ports */
void mml_set_error_handler(void (*handler)(const char* err, int line, int column));

/*! 
 * Parse the MML file (entirely read and passed to `content`) and produce 
 * an offline set of frames by channel (frame map).
 * The returned set can be transformed in a sequential stream
 * by `seq_compile`.
 * Returns non-zero in case of parse error.
 */
int mml_compile(const char* content, struct seq_frame_map_t* map);

/*!
 * Free the map allocated by `mml_compile`.
 */
void mml_free(struct seq_frame_map_t* map);

#endif