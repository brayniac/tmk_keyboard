/*
Copyright 2012 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * scan matrix
 */
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"


#ifndef DEBOUNCE
#   define DEBOUNCE	5
#endif
static uint8_t debouncing = DEBOUNCE;

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

static matrix_row_t read_cols(void);
static void init_cols(void);
static void unselect_rows(void);
static void select_row(uint8_t row);


inline
uint8_t matrix_rows(void)
{
    return MATRIX_ROWS;
}

inline
uint8_t matrix_cols(void)
{
    return MATRIX_COLS;
}

void matrix_init(void)
{

    UCSR0B = 0; //Turn off serial 

    // initialize row and col
    unselect_rows();
    init_cols();

    // initialize matrix state: all keys off
    for (uint8_t i=0; i < MATRIX_ROWS; i++) {
        matrix[i] = 0;
        matrix_debouncing[i] = 0;
    }
    
}

uint8_t matrix_scan(void)
{
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        select_row(i);
        _delay_us(100);  // without this wait read unstable value.
        matrix_row_t cols = read_cols();
        if (matrix_debouncing[i] != cols) {
            matrix_debouncing[i] = cols;
            if (debouncing) {
                debug("bounce!: "); debug_hex(debouncing); debug("\n");
            }
            debouncing = DEBOUNCE;
        }
        unselect_rows();
    }

    if (debouncing) {
        if (--debouncing) {
            _delay_ms(1);
        } else {
            for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
                matrix[i] = matrix_debouncing[i];
            }
        }
    }

    return 1;
}

bool matrix_is_modified(void)
{
    if (debouncing) return false;
    return true;
}

inline
bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & ((matrix_row_t)1<<col));
}

inline
matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

void matrix_print(void)
{
    print("\nr/c 0123456789ABCDEF\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        phex(row); print(": ");
        pbin_reverse16(matrix_get_row(row));
        print("\n");
    }
}

uint8_t matrix_key_count(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        count += bitpop16(matrix[i]);
    }
    return count;
}

/* Column pin configuration
 * col: 0   1   2   3   4   5   6   7   8   9   10  11 
 * pin: B0  B1  B2  C0  C1  C2  C3  C4  C5  D0  D1  B4
 */
 
static void  init_cols(void)
{
    // Input with pull-up(DDR:0, PORT:1)
    DDRB  &= ~(1<<0 | 1<<1 | 1<<2 | 1<<4);
    PORTB |=  (1<<0 | 1<<1 | 1<<2 | 1<<4);
    DDRC  &= ~(1<<0 | 1<<1 | 1<<2 | 1<<3 | 1<<4 | 1<<5);
    PORTC |=  (1<<0 | 1<<1 | 1<<2 | 1<<3 | 1<<4 | 1<<5);
    DDRD  &= ~(1<<0 | 1<<1);
    PORTD |=  (1<<0 | 1<<1);
}

static matrix_row_t read_cols(void)
{
    return (PINB&(1<<0) ? 0 : (1<<0)) |
           (PINB&(1<<1) ? 0 : (1<<1)) |
           (PINB&(1<<2) ? 0 : (1<<2)) |
           (PINC&(1<<0) ? 0 : (1<<3)) |
           (PINC&(1<<1) ? 0 : (1<<4)) |
           (PINC&(1<<2) ? 0 : (1<<5)) |
           (PINC&(1<<3) ? 0 : (1<<6)) |
           (PINC&(1<<4) ? 0 : (1<<7)) |
           (PINC&(1<<5) ? 0 : (1<<8)) |
           (PIND&(1<<0) ? 0 : (1<<9)) |
           (PIND&(1<<1) ? 0 : (1<<10)) |
           (PINB&(1<<4) ? 0 : (1<<11));
}

/* Row pin configuration
 * row: 0   1   2   3
 * pin: D6  D5  D3  B3
 */
 
static void unselect_rows(void)
{
    // Hi-Z(DDR:0, PORT:0) to unselect
    DDRB  &= ~0b00001000;
    PORTB &= ~0b00001000;
    DDRD  &= ~0b01101000;
    PORTD &= ~0b01101000;

}

static void select_row(uint8_t row)
{
    // Output low(DDR:1, PORT:0) to select
    switch (row) {
        case 0:
            DDRD  |= (1<<6);
            PORTD &= ~(1<<6);
            break;
        case 1:
            DDRD  |= (1<<5);
            PORTD &= ~(1<<5);
            break;
        case 2:
            DDRD  |= (1<<3);
            PORTD &= ~(1<<3);
            break;
	case 3:
	    DDRB  |= (1<<3);
	    PORTB &= ~(1<<3);
	    break;
    }
}
