#!/usr/bin/env python

"""
Polyphonic synthesizer for microcontrollers: Sine look-up table generator
(C) 2017 Stuart Longland

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
MA  02110-1301  USA
"""

import math
import textwrap
import argparse

# Arguments
parser = argparse.ArgumentParser(
        description='Generate a sine wave look-up table in C'
)
parser.add_argument('--num-samples-bits',
        help='--num-samples is in bits, so # samples = 2^n, allowing a '\
                'bitwise AND to be used for modulus operations and bit '\
                'shifting for segment decoding.',
        action='store_const', default=False, const=True)
parser.add_argument('--num-samples',
        help='Number of samples to generate',
        type=int, default=10)
parser.add_argument('--amplitude',
        help='Amplitude of sine to generate',
        type=int, default=255)
parser.add_argument('--data-type',
        help='Data type of sine samples',
        type=str, default='uint8_t')
args = parser.parse_args()

# Number of samples
if args.num_samples_bits:
    num_samples = 2 ** args.num_samples
else:
    num_samples = args.num_samples

# Step
step_angle = math.pi / (2*num_samples)

# Samples
samples = [int(args.amplitude*math.sin(sample * step_angle))
        for sample in range(0, num_samples)]

# Output C file
print ("""#ifndef _GEN_SINE_C
#define _GEN_SINE_C

/* Generated sine wave */
#include <stdint.h>
#define POLY_SINE_SZ (%(N_SAMPLES)d)
#define POLY_SINE_SZ_BITS (%(N_BITS)d)
static const %(DATA_TYPE)s _poly_sine[POLY_SINE_SZ]
#ifdef __AVR_ARCH__
PROGMEM
#endif
= {
%(SAMPLES)s
};
#endif""" % {
    'N_SAMPLES': num_samples,
    'N_BITS': args.num_samples if args.num_samples_bits else 0,
    'DATA_TYPE': args.data_type,
    'SAMPLES': '\n'.join(
        textwrap.TextWrapper(
            width=78,
            initial_indent='    ',
            subsequent_indent='    '
        ).wrap(
            ', '.join([
                ('0x%02x' % sample) for sample in samples
            ])
        )
    )
})
