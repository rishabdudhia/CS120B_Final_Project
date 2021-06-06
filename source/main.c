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

enum BlinkStates { bstart, blink_off, blink } blstate;
int Blink_Tick (int blstate) {
	//unsigned char output = 0x00;
	switch (blstate) {
		case bstart:
			blstate = blink_off;
			//output = 0x00;
			break;
		case blink_off:
			if (start_blink != 0 && bet_ready != 0) {
				blstate = blink;
				curr_on = 0x01;
			}
			else 
				blstate = blink_off;
			break;
		case blink:
			if (start_blink != 0) {
				blstate = blink;
				if (curr_on == 0x10) 
					curr_on = 0x01;
				else
					curr_on = curr_on << 1;
			}
			else 
				blstate = blink_off;
			break;
		default:
			blstate = bstart;
			break;
	}

	switch (blstate) {
		case bstart:
			break;
		case blink_off:
		case blink:
			PORTA = curr_on;
			break;
		default:
			break;
	}

	return blstate;
}

enum DetectStartBlinkStates { dstart, waitA7, clickA7 } dstate;
int Detect_Tick (int dstate) {
	unsigned char in = ~PINC & 0x40;
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

enum BetStates { startb, bwait, inc, dec, lock, points, reset } bstate;
int Bet_Tick (int bstate) {
	unsigned char in = ~PINC & 0xA0;
	unsigned char inA = ~PINA & 0x20;
	switch (bstate) {
		case startb:
			bstate = bwait;
			curr_points = 2;
			curr_bet = 1;
			bet_ready = 0;
			total_points = curr_points + curr_bet;
			break;
		case bwait:
			if (inA == 0 && start_blink == 0 && in == 0x80 && curr_bet < total_points) {
				bstate = inc;
				curr_bet = curr_bet + 1;
				curr_points = curr_points - 1;
			}
			else if (inA == 0 && start_blink == 0 && in == 0x20 && curr_bet > 1) {
				bstate = dec;
				--curr_bet;
				curr_points = curr_points + 1;
			}
			else if (inA == 0 && start_blink != 0) {
				bet_ready = 1;
				bstate = lock;
			}
			else if (inA != 0) 
				bstate = reset;
			else
				bstate = bwait;
			break;
		case reset:
			if (inA != 0) 
				bstate = reset;
			else
				bstate = bwait;
			break;
		case inc:
			if (in == 0x80)
				bstate = inc;
			else
				bstate = bwait;
			break;
		case dec:
			if (in == 0x20) 
				bstate = dec;
			else
				bstate = bwait;
			break;
		case lock:
			if (start_blink == 0) {
				bstate = points;
				rewards = 1;
			}
			else
				bstate = lock;
			break;
		case points:
			bstate = bwait;
			curr_bet = 1;
			curr_points = curr_points - curr_bet;
			break;
		default:
			bstate = startb;
			break;
	}

	switch (bstate) {
		case startb:
		case inc:
		case dec:
		case lock:
		case points:
			break;
		case bwait:
			bet_ready = 0;
			total_points = curr_points + curr_bet;
			break;
/*		case reset:
			curr_points = 2;
			curr_bet = 1;
			curr_on = 0x00;
			break;
*/		default:
			break;
	}

	return bstate;
}

enum AwardStates { astart, bet_mode, reward } astate;
int Award_Tick (int astate) {
	switch (astate) {
		case astart:
			astate = bet_mode;
			break;
		case bet_mode:
			if (rewards == 0)
				astate = bet_mode;
			else
				astate = reward;
			break;
		case reward:
			astate = bet_mode;
			rewards = 0;
			break;
		default:
			astate = astart;
			break;
	}

	switch (astate) {
		case astart:
		case bet_mode:
			break;
		case reward:
			if (curr_on == 0x04) 
				curr_bet = 2 * curr_bet;
			else
				curr_bet = 0;
			curr_points = curr_points + curr_bet;
			break;
		default:
			break;
	}

	return astate;
}


enum SevSegStates { sevstart, disp_points } sevstate;
int SevSeg_Tick (int sevstate) {
	Write7Seg(curr_points); //change to curr_points
	return sevstate;
}

int Matrix_Tick(int state) {
	static unsigned char i = 0;
	static unsigned char k = 0;
	static unsigned char r = 0x00;
	static unsigned char c = 0x00;
	static unsigned char rows[5] = {0x01, 0x03, 0x07, 0x0F, 0x1F};
	unsigned char pts[2] = {0x0F, 0xFF};

	if (curr_bet > 5) {
		i = 1;
		k = 4;
	}
	else {
		i = 0;
		k = curr_bet - 1;
	}
	r = rows[k];

	/*for (unsigned char j = 1; j < 6; ++j) {
		if (j <= curr_bet) {
			r = r << 1;
			++r;
		}
		else
			break;
	}*/
	PORTC = ~r;
	c = pts[i];
	PORTD = c;
	return 0;
}
/*
enum ResetStates { rstart, waitr, reset } rstate;
int Reset_Tick (int rstate) {
	unsigned char in = ~PINA & 0x20;
	switch (rstate) {
		case rstart:
			rstate = waitr;
			break;
		case waitr:
			if (in == 0) 
				rstate = waitr;
			else
				rstate = reset;
			break;
		case reset:
			if (in == 0)
				rstate = waitr;
			else
				rstate = reset;
			break;
		default:
			rstate = rstart;
			break;
	}

	switch (rstate) {
		case rstart:
		case waitr:
			break;
		case reset:
			curr_points = 2;
			curr_bet = 1;
			curr_on = 0x00;
			break;
		default:
			break;
	}

	return rstate;
}
*/

int main(void) {
    /* Insert DDR and PORT initializations */

	DDRA = 0x1F; PORTA = 0xE0;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0x1F; PORTC = 0xE0;
	DDRD = 0xFF; PORTD = 0x00;

	static task task1, task2, task3, task4, task5, task6, task7;

	task *tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6, &task7 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	const char start = -1;
	//Task 1
	task1.state = start;
	task1.period = 50;
	task1.elapsedTime = task1.period;
	task1.TickFct = &Detect_Tick;

	//Task 2
	task2.state = start;
	task2.period = 150;
	task2.elapsedTime = task2.period;
	task2.TickFct = &Blink_Tick;

	//Task 3
	task3.state = start;
	task3.period = 50;
	task3.elapsedTime = task3.period;
	task3.TickFct = &SevSeg_Tick;

	//Task 4
	task4.state = start;
	task4.period = 50;
	task4.elapsedTime = task4.period;
	task4.TickFct = &Matrix_Tick;

	//Task 5
	task5.state = start;
	task5.period = 50;
	task5.elapsedTime = task5.period;
	task5.TickFct = &Bet_Tick;

	//Task 6
	task6.state = start;
	task6.period = 50;
	task6.elapsedTime = task6.period;
	task6.TickFct = &Award_Tick;

/*	//Task 7
	task7.state = start;
	task7.period = 50;
	task7.elapsedTime = task7.period;
	task7.TickFct = &Reset_Tick;
*/
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
