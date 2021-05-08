ADSR-based Polyphonic Synthesizer
=================================

This project is intended to be a polyphonic synthesizer for use in
embedded microcontrollers.  It features multi-voice synthesis for
multiple channels.

The synthesis is inspired from the highly regarded MOS Technologies 6581
"SID" chip, which supported up to 3 voices each producing either a
square wave, triangle wave or sawtooth wave output and hardware
attack/decay/sustain/release envelope generation.

This tries to achieve the same thing in software.

Principle of operation
----------------------

The library runs as a state machine.  Synthesis is performed completely
using only integer arithmetic operations: specifically addition,
subtraction, left and right shifts, and occasional multiplication.  This
makes it suitable for smaller CPU cores such as Atmel's TinyAVR, ARM's
Cortex M0+, lower-end TI MSP430 and other minimalist CPU cores that lack
hardware multipliers or floating-point hardware.

The data types and sizes are optimised for 8-bit microcontroller
hardware.

The state is defined as an array of "voice" objects, all of the type
`struct voice_ch_t` and a synthesizer state machine object of type
`struct poly_synth_t`.

These voices combine an ADSR envelope generator and a waveform
generator.  A voice is configured by setting the waveform type and
frequency in the waveform generator.  This algorithmically provides a
monophonic tone which is then amplitude-modulated using the ADSR
envelope generator.

Under the control of the synthesizer state machine, the voices are
selectively computed and summed to produce a final sample value for the
output.  The bit masks that enable and mute channels are defined by the
`uintptr_t` data type, and so in most cases, 16 or 32 channels can be
accommodated depending on the underlying microcontroller.

### ADSR Envelope

ADSR stands for Attack-Decay-Sustain-Release, and forms a mechanism for
modelling typical instrument sounds.  The state machine for the ADSR
waveform moves through the following states:

1. Delay phase: This is a programming convenience that allows for the
   state of multiple voices to be configured at some convenient point in
   the program in bulk whilst still providing flexibility on when a
   particular note is played.  As there is no amplitude change, an
   "infinite" time delay may also be specified here, allowing a note to
   be configured then triggered "on cue".

2. Attack phase: The amplitude starts at 0, and using an approximated
   exponential function, rises rapidly up to the peak amplitude.  The
   exponential function is approximated by left-shifting the peak
   amplitude value by varying amounts.

3. Decay phase: The amplitude drops from the peak, to the sustain
   amplitude.  The decay is linear with time and is achieved by
   subtracting a fraction of the difference between peak and sustain
   amplitudes each cycle.

4. Sustain phase: The amplitude is held constant at the sustain
   amplitude.  As there is no amplitude change, it is also possible to
   define this with an "infinite" duration, allowing the note to be
   released "on cue" (e.g. when the user releases a key).

5. Release phase: The amplitude dies off with a reverse-exponential
   function much like the attack phase.  Again, it is approximated by
   left-shifting the sustain amplitude.

Typical usage
-------------

The typical usage scenario is to statically define an array of `struct
voice_ch_t` objects and a `struct poly_synth_t` object.  To set the
sample rate, create a header file with the content:

```
#define SYNTH_FREQ		(16000)
```

… then in your project's `Makefile` or C-preprocessor settings, define
`SYNTH_CFG=\"synth-config.h\"` to tell the library where to find its
configuration.

The above example sets the sample rate to 16kHz… you can set any value
here appropriate for your microcontroller.

The `struct poly_synth_t` is initialised by clearing the `enable` and
`mute` members, and setting the `voice` member to the address of the
array of `struct voice_ch_t` objects.

Having initialised the data structures, you can then start reading your
musical score.  To play a note, you select a voice channel, then:

* Call `adsr_config` with the arguments:
  * `time_scale`: number of samples per "time unit"
  * `delay_time`: number of `time_scale` units before the "attack"
    phase.  If set to `ADSR_INFINITE`, the note is delayed until
    `adsr_continue` is called.
  * `attack_time`: number of `time_scale` units taken for the note to
    reach peak amplitude (`peak_amp`)
  * `decay_time`: number of `time_scale` units taken for the note to
    decay to the "sustain" amplitude (`sustain_amp`)
  * `sustain_time`: number of `time_scale` units taken for the note to
    hold the `sustain_amp` amplitude before the "release" phase.
  * `release_time`: number of `time_scale` units taken for the note to
    decay back to silence.
  * `peak_amp`: the peak amplitude of the note.
  * `sustain_amp`: the sustained amplitude of the note.
* Call one of the waveform generator set-up functions.
  * `amplitude` sets the base amplitude for the waveform generator.
  * `freq` sets the frequency for the waveform generator.
* Set the corresponding bits in `struct poly_synth_t`:
  * set the corresponding `enable` bit to compute the output of that
    synth voice channel
  * clear the corresponding `mute` bit for the output of that synth
    channel to be added to the resultant output.

Then call `poly_synth_next` repeatedly to read off each sample.  The
samples are returned as signed 8-bit PCM.  Each call will advance the
state machines and so successive calls will return consecutive samples.

As each channel finishes, the corresponding bit in the `enable` member
of `struct poly_synth_t` is cleared.

When all the machines have finished, the `poly_synth_next` function will
return all zeros and the `enable` field of `struct poly_synth_t` will be
zero.
 
### Waveform generators

There are 5 waveform generator algorithms to choose from.  The state
machines have the following variables:

* `sample`: The latest waveform generator sample.
* `amplitude`: The peak waveform amplitude (from 0 axis, so half.
  peak-to-peak).
* `period`: The period of the internal state machine counter
* `period_remain`: The internal state machine counter itself.  This gets
  set to a value then decremented until it reaches zero.
* `step`: The amplitude step size.

#### DC waveform generator (`voice_wf_set_dc`)

Configures the waveform generator to generate a "DC" waveform (constant
amplitude).  Not terribly useful at this time but may be handy if you
wish to use the ADSR envelope generator only to modulate lights.

#### Square wave generator (`voice_wf_set_square`)

Configures the waveform generator to generate a square wave.

`sample` is initialised as `+amplitude`, and the half-period is
computed as `period=SYNTH_FREQ/(2*freq)`. `period_remain` is
initialised to `period`.

Fixed-point `12.4` format (16 bit) is used to store the period counters, to allow tuned notes on low sampling rates too.

Each sample, `period_remain` is decremented.  When `period_remain`
reaches zero:

* `sample` is algebraically negated
* `period_remain` is reset back to `period`

#### Sawtooth wave generator (`voice_wf_set_sawtooth`)
  
Configures the waveform generator to produce a sawtooth wave.

`sample` is initialised as `-amplitude`, and the time period is
computed as `period=SYNTH_FREQ/freq`.  The `step` is computed as
`step=(2*amplitude)/T`.  `period_remain` is initialised at `period`.

Every sample, `sample` is incremented by `step` and `period_remain`
decremented.  When `period_remain` reaches zero:

* `sample` is reset to `-amplitude`
* `period_remain` reset to `period`

#### Triangle wave generator (`voice_wf_set_triangle`)
  
Configures the waveform generator to produce a triangle wave.

`sample` is initialised as `-amplitude`, and the time period is
computed as `period=SYNTH_FREQ/(2*freq)`.  The `step` is computed as
`step=(2*amplitude)/T`.  `period_remain` is initialised at `period`.

Every sample, `sample` is incremented by `step` and `period_remain`
decremented.  When `period_remain` reaches zero:

* if `step` is negative, `sample` is reset to `-amplitude`, otherwise
  it is reset to `+amplitude`.
* `step` is algebraically negated.
* `period_remain` is reset to `period`

#### Pseudorandom noise generator (`wf_voice_set_noise`)

This generates random samples at a given amplitude.  The randomness
depends on the C library's random number generator (`rand()`), so it may
help to periodically seed it, perhaps by taking the least-significant
bits of ADC readings and feeding those into `srand` to give it some true
randomness.

Ports
-----

The code is written with portability in mind.  While it has only ever
been compiled on GNU/Linux platforms, it has successfully worked on AVR
and Linux/x86-64.  The code *should* compile and work for other
processor architectures too.

To build a port, run:

```
$ make PORT=port_name
```

### ATTiny85 port (`attiny85`)

The ATTiny85 port was the first microcontroller port of this synthesizer
library.  The demonstration port tries to operate as a stand-alone
synthesizer.  The PWM output is emitted out of `PB4` and `PB3` is used
as an ADC input.

Connected to `PB3` is a voltage divider network, with the segments
connected to Vcc via pushbuttons.  When a button is pressed, it shorts a
section of the resistor divider out, and a higher voltage is seen on the
ADC input.

The voltage is translated to a button press and used to trigger one of
the voices, each of which have been configured with a different note.

The code is a work-in-progress, with some notable bugginess with the
ADC-based keyboard implementation.

The remaining pins are available for other uses such as I²C/SPI or for
additional GPIOs.

### ATTiny861 port (`attiny861`)

This code is forked from the ATTiny85 port when it was realised that the
ATTiny85 with its 5 usable I/O pins would be incapable of driving lots
of lights without a lot of I/O expansion hardware.

Here, four I/O pins on port B are allocated:

* `PB3`: PWM audio output
* `PB4`: audio enable pin
* `PB5`: PWM light output
* `PB6`: GPIO enable pin

The audio amplifier used in the prototype is the NJR NJM2113D, and
features a "chip disable" pin that powers down the amplifier when pulled
high.  The audio enable signal drives a N-channel MOSFET (e.g. 2N7000)
that pulls the pin low against a pull-up resistor to +12V.

A logic high on the audio enable signal turns on the amplifier.

All of the pins on port A (`PA0` through to `PA7`) are used for
interfacing to MOSFETs and pushbuttons by way of a multiplexing circuit
driven by the GPIO enable pin.  The multiplexing circuit consists of two
4066-style analogue switches (I am using Motorola MC14066Bs here because
I have dozens of them with 8241 date codes) and a 74374 D-latch.

The GPIO enable line connects the clock input of the 74374 and the
switch enable pins on all switches in both 4066s.  The 4066 and 74374
inputs are paralleled.

When GPIO enable is driven low, this turns off the 4066s allowing us to
assert signals for the 74374.  On the rising edge of GPIO enable, the
74374 latches those signals and the 4066s re-connect port A to the
outside world.  By doing this, port A is able to be used both for
control of digital outputs hanging off the 74374 and for analogue +
digital I/O through the 4066s.

The light PWM signal on `PB5` connects to the 74374's "output enable"
line, and thus by using pull-down resistors on the outputs of the 74374,
it can drive N-channel MOSFETs to control the brightness of 8 lights.

Individual control of lights can be achieved with ⅛ maximum duty cycle
by choosing a single output then driving that line with the desired PWM
amplitude.

### PC port (`pc`)

This uses `libao` and a command line interface to simulate the output of
the synthesizer and to output a `.wav` file.  It was used to debug the synthesizer.

The synthesizer commands are given as command-line arguments:

* `voice V` selects a given voice channel
* `mute M` sets the synthesizer `mute` bit-mask
* `en M` sets the synthesizer `enable` bit-mask
* `dc A` sets the selected voice channel to produce a DC offset of
  amplitude `A`.
* `noise A` sets the selected voice channel to produce pseudorandom
  noise at amplitude `A`.
* `square F A` sets the selected voice channel to produce a square wave
  of frequency `F` Hz and amplitude `A`.
* `sawtooth F A` sets the selected voice channel to produce a sawtooth
  wave of frequency `F` Hz and amplitude `A`.
* `triangle F A` sets the selected voice channel to produce a triangle
  wave of frequency `F` Hz and amplitude `A`.
* `scale N` sets the ADSR time unit scale for the selected channel to
  `N` samples per "tick"
* `delay N` sets the ADSR delay period for the selected channel to `N`
  "ticks"
* `attack N` sets the ADSR attack period for the selected channel to `N`
  "ticks"
* `decay N` sets the ADSR decay period for the selected channel to `N`
  "ticks"
* `sustain N` sets the ADSR sustain period for the selected channel to `N`
  "ticks"
* `release N` sets the ADSR release period for the selected channel to `N`
  "ticks"
* `peak A` sets the peak ADSR amplitude for the selected channel to `A`
* `samp A` sets the sustained ADSR amplitude for the selected channel to
  `A`
* `reset` resets the ADSR state machine for the selected channel.

Or, alternatively, you can pass all the above commands stored in a text file:
* `-- NAME` loads and run the script, and skip all the remaining arguments.

Once the `enable` bit-mask is set, the program loops, playing sound via
`libao` and writing the samples to `out.wav` for later analysis until
all bits in the `enable` bit-mask are cleared by the ADSR state machines.

When the program runs out of command line arguments, or the script ends, it exits.
