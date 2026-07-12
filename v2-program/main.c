/*
 * main.c
 *
 * Created: 11/21/2024 11:37:21 PM
 * Author : Kyle Schmerge, Daniel Van Dalsem
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// For setting individual bits
#define sbi(x,b) (x) |= 1<<(b)
#define cbi(x,b) (x) &= ~(1<<(b))

#define LED_PIN 6
#define SWITCH_PIN PCINT5
#define STRIP_LEN 47
#define NUM_PROGRAMS 6

#define TCC0 0
#define TCC1 1

#define DEBOUNCE_TIME_MS 25UL

// Order is important! colors are sent as g, r, b
typedef struct Color {
	unsigned char g;
	unsigned char r;
	unsigned char b;
} Color;

Color hsv2rgb(float, float, float);

typedef struct xorshift32_state {
	uint32_t a;
} xorshift32_state;

const Color red = { .r = 128, .g = 0, .b = 0 };
const Color white = { .r = 128, .g = 128, .b = 128 };
const Color gold = { .r = 255, .g = 255, .b = 0 };
const Color green = { .r = 0, .g = 255, .b = 0 };
const Color lush_green = { .r = 50, .g = 168, .b = 82 };
const Color off = { .r = 0, .g = 0, .b = 0 };

static Color strip[STRIP_LEN];
static bool candy_state[STRIP_LEN] = { false };
static volatile int program = 0;

static volatile uint32_t millis;

/* The state must be initialized to non-zero */
static xorshift32_state randomstate = { .a = 0x75AE3D9A };

extern void output_grb(uint8_t * ptr, uint16_t count);
extern void reset();

#define DEBOUNCE_DELAY 100

#define SATURATION 100.0f
#define INTENSITY 100.0f

Color rainbow[STRIP_LEN];

uint32_t xorshift32() {
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = randomstate.a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return randomstate.a = x;
}

bool randbool() {
	return xorshift32() % 2 == 0;
}

void setup() {
	// Set LED Pin as output
	sbi(DDRB, LED_PIN);
	cbi(PORTB, LED_PIN);
	// Set inputs for detecting usb power mode
	DDRC &= ~(1 << PC2);
	DDRC &= ~(1 << PC4);
	DDRC &= ~(1 << PC5);
	DDRC &= ~(1 << PC6);
	// cbi(DDRD, TCC0);
	// cbi(DDRD, TCC1);
	// cbi(PORTD, TCC0);
	// cbi(PORTD, TCC1);
	
	// Pin change interrupt on mode button
	// PCMSK0 = (1 << SWITCH_PIN);
	// PCICR = (1 << PCIE0);
	
	// Set program change switch pin as input and engage pull-up resistor
	// cbi(DDRB, SWITCH_PIN);
	// sbi(PORTB, SWITCH_PIN);
	DDRD &= ~(1 << DDD0);  // Set PD0 as Input
    PORTD |= (1 << PORTD0);  // Enable internal pull-up resistor
	EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
	EIMSK |= (1 << INT0);

	
	// 1kHz millis interrupt
	TCCR0A = (1 << WGM01);
	OCR0A = 249;
	TCCR0B = (1 << CS01) | (1 << CS00);
	TIMSK0 = (1 << OCIE0A);
	
	// Precompute rainbow
	for(int i = 0; i < STRIP_LEN; i++)
		rainbow[i] = hsv2rgb(360.0f * (STRIP_LEN - i - 1) / STRIP_LEN, SATURATION, INTENSITY);
	
	for (int i = 0; i < STRIP_LEN - 1; i++) {
		candy_state[i] = randbool();
	}
	
	sei();
	
}

ISR(TIMER0_COMPA_vect) {
	millis++;
}

ISR(INT0_vect) {
    _delay_ms(DEBOUNCE_TIME_MS);

    if (bit_is_clear(PIND, PIND0)) {
		program += 1;
		if (program > 4) {
			program = 0;
		}
    }
    
    EIFR |= (1 << INTF0);
}

// ISR(PCINT0_vect) {
// 	static uint32_t last_change = 0;
// 	if(!(PINB & (1 << SWITCH_PIN)) && millis - last_change > DEBOUNCE_DELAY)
// 		program++;
// 	last_change = millis;
// }

Color hsv2rgb(float H, float S, float V) {
	float r = 0, g = 0, b = 0;
	
	float h = H / 360;
	float s = S / 100;
	float v = V / 100;
	
	int i = floor(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	
	switch (i % 6) {
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}
	
	Color color;
	color.r = r * 255;
	color.g = g * 255;
	color.b = b * 255;
	
	return color;
}

void show(Color * colors, int colors_len) {
	output_grb((uint8_t*) colors, colors_len * sizeof(Color));
	reset();
}

void set_solid(Color c, Color * colors, int colors_len) {
	for (int i = 0; i < colors_len; i++) {
		colors[i] = c;
	}
}

void candy_cane() {
	strip[STRIP_LEN - 1] = gold;
	for (int i = 0; i < STRIP_LEN - 1; i++) {
		strip[i] = randbool() ? red : white;
	}
	show(strip, STRIP_LEN);
	_delay_ms(500);
}

void candy_cane2() {
	int flip = xorshift32() % STRIP_LEN;
	candy_state[flip] = !candy_state[flip];
	for (int i = 0; i < STRIP_LEN; i++) {
		strip[i] = candy_state[i] ? red : white;
	}
	show(strip, STRIP_LEN);
	_delay_ms(500);
}

void solid_green() {
	strip[STRIP_LEN - 1] = gold;
	set_solid(green, strip, STRIP_LEN - 1);
	show(strip, STRIP_LEN);
	_delay_ms(250);
}

void solid_white() {
	strip[STRIP_LEN - 1] = gold;
	set_solid(white, strip, STRIP_LEN - 1);
	show(strip, STRIP_LEN);
	_delay_ms(250);
}

static int white_wave_state = 0;

#define STAR_NUM_BLINKS 3

void white_wave() {
	for (int i = 0; i < STRIP_LEN; i++) {
		if (i == (STRIP_LEN - 1) && white_wave_state >= (STRIP_LEN - 1)) {
			strip[i] = white_wave_state % 2 == 0 ? off : gold;
		}
		else if (i == white_wave_state) {
			strip[i] = white;
		}
		else {
			strip[i] = off;
		}
	}
	show(strip, STRIP_LEN);
	_delay_ms(250);
	white_wave_state++;
	white_wave_state %= (STRIP_LEN + STAR_NUM_BLINKS * 2 - 1);
}

static bool green_red_state = false;

void green_red() {
	strip[STRIP_LEN - 1] = gold;
	for (int i = 0; i < STRIP_LEN - 1; i++) {
		if ((i + green_red_state) % 2 == 0) {
			strip[i] = green;
		}
		else {
			strip[i] = red;
		}
	}
	show(strip, STRIP_LEN);
	_delay_ms(250);
	green_red_state = !green_red_state;
}

static float hue_val = 0.0;

void rgb() {
	//strip[STRIP_LEN - 1] = gold;
	Color c = hsv2rgb(hue_val, SATURATION, INTENSITY);
	for (int i = 0; i < STRIP_LEN; i++) {
		strip[i] = c;
	}
	show(strip, STRIP_LEN);
	_delay_ms(25);
	hue_val += 1;
	hue_val = fmod(hue_val, 360);
}

void cycle() {
	for(int i = 0; i < STRIP_LEN; i++)
		strip[i] = hsv2rgb(360 - fmod(hue_val + 360.0f * i / STRIP_LEN, 360), SATURATION, INTENSITY);
	show(strip, STRIP_LEN);
	_delay_ms(10);
	hue_val += 10;
	hue_val = fmod(hue_val, 360);
}

void bugged_but_cool_cycle() {
	static float fade_pos = 0;

	for(int i = 0; i < STRIP_LEN; i++) {
		float pos = (float)i / STRIP_LEN;
		float arg = (fade_pos - 2 * (pos - 0.5f));
		strip[i] = hsv2rgb(fmod(hue_val + 360.0f * pos, 360), SATURATION, 100 * powf(0.5 * (sin(arg * 2 * M_PI) + 1), 5));
	}
	show(strip, STRIP_LEN);
	_delay_ms(10);
	
	hue_val += 10;
	hue_val = fmod(hue_val, 360);
	
	fade_pos += 0.01;
	if(fade_pos > 1)
		fade_pos = 0;
}

void death() {
	static int offset = 0;
	static char doom = 0;
	
	offset = xorshift32() % STRIP_LEN;
	
	if(doom) {
		for(int i = 0; i < STRIP_LEN; i++) {
			int ind = (offset + i) % STRIP_LEN;
			strip[i] = rainbow[ind];
		}
		offset = (offset + 1) % STRIP_LEN;
		doom = 0;
	} else {
		for(int i = 0; i < STRIP_LEN; i++) {
			strip[i].r = 0;
			strip[i].g = 0;
			strip[i].b = 0;
		}
		doom = 1;
	}
	
	show(strip, STRIP_LEN);
	
	_delay_ms(25);
}

void err() {
	// Turn all LEDs off if the USB port cannot provide adequate power
	memset(strip, 0, sizeof(Color) * STRIP_LEN);
	strip[0] = red;
	show(strip, STRIP_LEN);
	_delay_ms(25);
}

int main(void) {
	
	// Initialize IO
	setup();
	
	// Set colors to all zero
	memset(strip, 0, sizeof(Color) * STRIP_LEN);
	
	while (1) {
		if ((PINC & (1 << PC4)) || (PINC & (1 << PC6)) || 1) {
			switch(program) {
				case 0:
					cycle();
					break;

				case 2:
					rgb();
					break;
					
				case 3:
					bugged_but_cool_cycle();
					break;
					
				case 4:
					death();
					break;
					
				default:
					program = 0;
					break;
			}
		} else {
			err();
		}
	}
}