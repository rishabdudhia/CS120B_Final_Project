/*	Author: Rishab Dudhia
 *  Partner(s) Name: 
 *	Lab Section: 022
 *	Assignment: Lab #11  Exercise #1
 *	Exercise Description: [optional - include for your own benefit]
 *	Gambling Game: 
 *	5 LEDs will light one at a time in a row. The player's goal is to press the stop
 *	button while the middle LED is lit. Before the round begins, the player will
 *	determine how many points they wish to bet from their "bank" which will be 
 *	displayed on the LED matrix. They will press buttons to bet points from their bank, 
 *	and their bet will be taken from their bank and the number of their bet will be 
 *	displayed on the seven segment display. The player can increase or decrease their bet 
 *	by the increase and decrease buttons, respectively. 
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif
#include "../header/seven_seg.h"
#include "../header/timer.h"

typedef struct _task {
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TickFct)(int);
} task;

unsigned char curr_points = 2;
unsigned char curr_bet = 1;
unsigned char total_points = 3;
unsigned char bet_ready = 1;
unsigned char curr_on = 0x00;
unsigned char start_blink = 0;
unsigned char rewards = 0;
unsigned char round_win = 0;

enum BlinkStates { bstart, blink_off, blink } bstate;
int Blink_Tick (int bstate) {
	unsigned char output = 0x00;
	switch (bstate) {
		case bstart:
			bstate = blink_off;
			output = 0x00;
			break;
		case blink_off:
			if (start_blink != 0 && bet_ready != 0) {
				bstate = blink;
				curr_on = 0x01;
			}
			else 
				bstate = blink_off;
			break;
		case blink:
			if (start_blink != 0) {
				bstate = blink;
				if (curr_on == 0x10) 
					curr_on = 0x01;
				else
					curr_on = curr_on << 1;
			}
			else 
				bstate = blink_off;
			break;
		default:
			bstate = bstart;
			break;
	}

	switch (bstate) {
		case bstart:
			break;
		case blink_off:
		case blink:
			PORTA = curr_on;
			break;
		default:
			break;
	}

	return bstate;
}

enum DetectStartBlinkStates { dstart, waitA7, clickA7 } dstate;
int Detect_Tick (int dstate) {
	unsigned char in = ~PINA & 0x80;
	switch (dstate) {
		case dstart:
			dstate = waitA7;
			break;
		case waitA7:
			if (in == 0x00) 
				dstate = waitA7;
			else {
				dstate = clickA7;
				if (start_blink == 0) 
					start_blink = 1;
				else
					start_blink = 0;
			}
			break;
		case clickA7:
			if (in == 0x00) 
				dstate = waitA7;
			else 
				dstate = clickA7;
			break;
		default:
			dstate = dstart;
			break;
	}

	switch (dstate) {
		case dstart:
		case waitA7:
		case clickA7:
		default:
			break;
	}

	return dstate;
}






int main(void) {
    /* Insert DDR and PORT initializations */

	DDRA = 0x1F; PORTA = 0xE0;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0x1F; PORTC = 0xE0;
	DDRD = 0xFF; PORTD = 0x00;

	static task task1, task2;

	task *tasks[] = { &task1, &task2 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	const char start = -1;
	//Task 1
	task1.state = start;
	task1.period = 50;
	task1.elapsedTime = task1.period;
	task1.TickFct = &Detect_Tick;

	//Task 2
	task2.state = start;
	task2.period = 50;
	task2.elapsedTime = task2.period;
	task2.TickFct = &Blink_Tick;

	TimerSet(50);
        TimerOn();

	unsigned short i;
    /* Insert your solution below */
	while (1) {
		for (i = 0; i < numTasks; ++i) {
			if (tasks[i]->elapsedTime == tasks[i]->period) {
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 50;
		}

		while (!TimerFlag);
		TimerFlag = 0;
	}
	return 1;
}
