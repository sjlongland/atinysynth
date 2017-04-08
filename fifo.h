#ifndef _UTIL_FIFO_H
#define _UTIL_FIFO_H

/*!
 * Simple ring FIFO buffer.
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
 * along with this program (see COPYING); if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <stdint.h>

/*!
 * Empty event.  Indicates that the buffer is now empty and the next read
 * will generate an underrun.
 */
#define FIFO_EVT_EMPTY		(1 << 0)

/*!
 * Underrun event flag, indicates that the consumer tried to read when the
 * buffer was empty.
 */
#define FIFO_EVT_UNDERRUN	(1 << 1)

/*!
 * Data arrived event.  Indicates that new data has arrived.
 */
#define FIFO_EVT_NEW		(1 << 2)

/*!
 * Buffer full event.  Indicates that the buffer is now full and the next
 * write will generate an overrun.
 */
#define FIFO_EVT_FULL		(1 << 3)

/*!
 * Overrun event flag, indicates that the producer tried to write when the
 * buffer was full.
 */
#define FIFO_EVT_OVERRUN	(1 << 4)

/*!
 * FIFO Buffer interface.
 */
struct fifo_t {
	/*! FIFO producer event handler */
	void (*producer_evth)(struct fifo_t* const fifo, uint8_t events);

	/*! FIFO consumer event handler */
	void (*consumer_evth)(struct fifo_t* const fifo, uint8_t events);

	volatile uint8_t* buffer;	/*!< Buffer storage location */
	uint8_t	total_sz;		/*!< Buffer total size */
	volatile uint8_t stored_sz;	/*!< Buffer usage size */
	volatile uint8_t read_ptr;	/*!< Read pointer location */
	volatile uint8_t write_ptr;	/*!< Write pointer location */

	uint8_t producer_evtm;		/*!< Producer event mask */
	uint8_t consumer_evtm;		/*!< Consumer event mask */

	void* producer_data;		/*!< Producer data pointer */
	void* consumer_data;		/*!< Consumer data pointer */
};

/*!
 * Execute one or more FIFO events.
 */
static void fifo_exec(struct fifo_t* const fifo, uint8_t events) {
	if (fifo->producer_evth && (fifo->producer_evtm & events))
		fifo->producer_evth(fifo, events);
	if (fifo->consumer_evth && (fifo->consumer_evtm & events))
		fifo->consumer_evth(fifo, events);
}

/*!
 * Empty the buffer.
 */
static void fifo_empty(struct fifo_t* const fifo) {
	fifo->stored_sz = 0;
	fifo->read_ptr = 0;
	fifo->write_ptr = 0;
}

/*!
 * Initialise the buffer
 */
static void fifo_init(struct fifo_t* const fifo,
		volatile uint8_t* buffer, uint8_t sz) {
	fifo_empty(fifo);
	fifo->buffer = buffer;
	fifo->total_sz = sz;
}

/*!
 * Read a byte from the buffer.  Returns the byte read, or -1 if no
 * data is available.
 */
static int16_t fifo_read_one(struct fifo_t* const fifo) {
	if (!fifo->stored_sz) {
		fifo_exec(fifo, FIFO_EVT_UNDERRUN);
		return -1;
	}

	uint8_t byte = fifo->buffer[fifo->read_ptr];
	fifo->stored_sz--;
	fifo->read_ptr = (fifo->read_ptr + 1) % fifo->total_sz;
	if (!fifo->stored_sz)
		fifo_exec(fifo, FIFO_EVT_EMPTY);
	return byte;
}

/*!
 * Read a byte from the buffer without consuming it.
 * Returns the byte read, or -1 if no data is available.
 */
static int16_t fifo_peek_one(struct fifo_t* const fifo) {
	if (!fifo->stored_sz)
		return -1;

	return fifo->buffer[fifo->read_ptr];
}

/*!
 * Write a byte to the buffer.  Returns 1 on success,
 * 0 if no space available.
 */
static uint8_t fifo_write_one(struct fifo_t* const fifo, uint8_t byte) {
	if (fifo->stored_sz >= fifo->total_sz) {
		fifo_exec(fifo, FIFO_EVT_OVERRUN);
		return 0;
	}

	fifo->buffer[fifo->write_ptr] = byte;
	fifo->stored_sz++;
	fifo->write_ptr = (fifo->write_ptr + 1) % fifo->total_sz;

	if (fifo->stored_sz)
		fifo_exec(fifo, FIFO_EVT_NEW);

	if (fifo->stored_sz == fifo->total_sz)
		fifo_exec(fifo, FIFO_EVT_FULL);
	return 1;
}

/*!
 * Read bytes from the buffer
 */
static uint8_t fifo_read(struct fifo_t* const fifo,
		uint8_t* buffer, uint8_t sz) {
	uint8_t count = 0;
	int16_t byte = fifo_read_one(fifo);
	while(sz && (byte >= 0)) {
		*buffer = byte;
		sz--;
		buffer++;
		count++;
		byte = fifo_read_one(fifo);
	}
	return count;
}

/*!
 * Read bytes from the buffer without consuming them.
 */
static uint8_t fifo_peek(struct fifo_t* const fifo,
		uint8_t* buffer, uint8_t sz) {
	uint8_t count = 0;
	uint8_t ptr = fifo->read_ptr;
	if (sz > fifo->stored_sz)
		sz = fifo->stored_sz;

	while(sz) {
		*buffer = fifo->buffer[ptr];
		sz--;
		buffer++;
		count++;
		ptr = (ptr + 1) % fifo->total_sz;
	}
	return count;
}

/*!
 * Write bytes to the buffer
 */
static uint8_t fifo_write(struct fifo_t* const fifo,
		const uint8_t* buffer, uint8_t sz) {
	uint8_t count = 0;
	while(sz && fifo_write_one(fifo, *buffer)) {
		buffer++;
		sz--;
		count++;
	}
	return count;
}

#endif
