//using port A as the inputs and D as the outputs
#include "lcd.h"
#include "ili934x.h"
#include "font.h"
#include "avrlcd.h"
#include "functions.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

void init_timer();
void info_display();
void display_current_time();
void power_reduction();
void draw_charge_symbol();
void clear_charge_symbol();

volatile bool record = 0;
volatile uint8_t duty_cycle;
uint8_t moving;

uint8_t i,j;
float u;
bool optimal_load_outcome[BITS];

volatile static uint32_t counter = 0;
volatile uint8_t interrupt;
uint16_t conversion, conversion2;
double current, current_scaled, volts, volts_scaled, power, power_scaled, wind, wind_scaled, solar, solar_scaled;

// Battery Storage Memory
volatile double BATTERY_CURRENT_STORED = 0;  
volatile double BATTERY_CURRENT_STORED_SCALED = 0;

// Digital Output Variables 
volatile bool  SW_L1 = 0; 
volatile bool  SW_L2 = 0; 
volatile bool  SW_L3 = 0; 
volatile bool  CHARGE_BATTERY = 0; 
volatile bool  DISCHARGE_BATTERY = 0; 

// Analogue Output Variables
volatile double  MAINS_I = 0;
volatile double  MAINS_I_REQ = 0; 

// Digital Input Variables 
volatile bool  CALL_L1 = 0; 
volatile bool  CALL_L2 = 0; 
volatile bool  CALL_L3 = 0; 

// Analogue Input Variables 
volatile double BUSBAR_V = 0;
volatile double BUSBAR_I = 0;
volatile double WIND_CAP = 0;
volatile double PV_CAP = 0;

double MAINS_I_SCALED, p_SCALED;

int smallest_index;
double sum;

unsigned long lastUpdateTime = 0;
uint8_t t;
float measurement;

float error;
float Kp = 0.05;

//Controller output
float out;

float timeElapsed;
unsigned long currentTime;
bool count_start;

double error_scaled;
float BUSBAR_CURRENT, BUSBAR_VOLTAGE;

bool CALL_L1_ARRAY[5];
bool CALL_L2_ARRAY[5];
bool CALL_L3_ARRAY[5];

float p;

ISR(TIMER0_OVF_vect) //interrupt service routine to increment the counter every second
{
	counter++;

	TCNT0 = 68; //set count register
}

ISR(TIMER1_OVF_vect) //interrupt service routine to adjust the duty cycle of the PWM pin
{
	OCR1A = duty_cycle;
}

ISR(TIMER1_COMPB_vect) //interrupt service routine to adjust the brightness of the TFT
{
    if(record == 0)
    {
		record = 1;
        BLC_hi();
    }
    else
    {
		record = 0;
        BLC_lo();
    }
}

int main()
{
	init_lcd();
	power_reduction();  //toggles some bits to shut down certain interfaces
	set_orientation(1);

	rectangle clear_screen = {.left=0, .right = (display.width-1), .top = 0, .bottom = display.height};
    fill_rectangle(clear_screen, BLACK);
	
	rectangle box1 = {.left = 0, .right = 160, .top = 0, .bottom=display.height};
	fill_rectangle(box1,YELLOW);
	rectangle box2 = {.left = 160, .right = 320, .top = 0, .bottom=display.height};
	fill_rectangle(box2,YELLOW);

	init_timer();  //sets up global timer
	init_adc();      //sets up ADC channels on ilmatto and analogue pins
	init_digital();  //sets up digital input and output pins
	setup_PWM();  //sets up PWM pins
	init_FIR_coeff();  //initialize FIR coefficients
	init_adc_buffer(); //initialize ADC buffer

	init_display_pacman(5, 0);
	init_ghost_red(0);
	init_ghost_green(0);           //initialisation of pacman and ghosts
	init_ghost_purple(0);
	init_ghost_orange(0);
	info_display_setup();

    change_position(6,8);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Team A", 1);       //displays team name in the top left hand corner

	sei(); //enables global interrupts
	_delay_ms(5000);
	while(return_BBI() < 0.5) //check to see if counter should start by monitoring busbar current
	{
		return_PWM(USAGE_L3);
		sw_load3(1);
	}	
	start_counter();
	while(1)
	{
		//dispays current info such as busbar current, busbar voltage, wind capacity and PV capacity
		display_current_time();
		algorithm();
	}
	return 0;
}

bool oversample(bool digitalInput, bool modeArray[])
{
	bool digitalOutput;
	modeArray[0] = digitalInput;

	sum = (double)(modeArray[0] + modeArray[1] + modeArray[2] + modeArray[3] + modeArray[4]) / 5.0;

	//shifts each value of modeArray to the right
	for (i = 4; i > 0; i--)
	{
		modeArray[i] = modeArray[i - 1];
	}
	
	//determines the mode of modeArray based on the calculated sum
	if (sum > 0.5)
	{
		digitalOutput = true;
	}
	else
	{
		digitalOutput = false;
	}
	return digitalOutput;
}

void start_counter()
{
	static bool counter_started = false; // Flag to indicate whether the counter has started

	if (!counter_started) 
	{
		// Start the counter only if it hasn't started yet
		counter_started = true;
		counter = 0; // Reset the counter(start of the algorithm)
		sw_load3(0);
		BATTERY_CURRENT_STORED = 0.0;
	}
}

void algorithm()
{
	currentTime = counter; // Get current time in milliseconds
	if(currentTime < lastUpdateTime) //for pin change interrupt when counter is reset back to 0/for start condition depending on 1st hour
	{
		timeElapsed = (lastUpdateTime - currentTime) / 1000.0f; // Convert milliseconds to seconds
	}
	else
	{
		timeElapsed = (currentTime - lastUpdateTime) / 1000.0f; // Convert milliseconds to seconds
	}

	char bufferString[10];
	bool opt_load_combination[BITS];
	
	//read in inputs
    PV_CAP = return_PVCap();
    WIND_CAP = return_windCap();
	CALL_L1 = call_load1();
	CALL_L2 = call_load2();
	CALL_L3 = call_load3();
	BUSBAR_VOLTAGE = return_BBV();
	BUSBAR_CURRENT = return_BBI();
	
	//oversampling
	static int k = 0;
	k++;
	
	if(k == 1)
	{
		for(j = 0; j < 5; j++)
		{
			CALL_L1_ARRAY[j] = CALL_L1;
			CALL_L2_ARRAY[j] = CALL_L2;
			CALL_L3_ARRAY[j] = CALL_L3;
		}
	}
	
	CALL_L1 = oversample(call_load1(), CALL_L1_ARRAY);
	CALL_L2 = oversample(call_load2(), CALL_L2_ARRAY);
	CALL_L3 = oversample(call_load3(), CALL_L3_ARRAY);

	optimal_choice(opt_load_combination);
	
	change_position(240,40);    //load 1 call
    change_foreground(BLACK);
    change_background(YELLOW);
	ltoa(CALL_L1, bufferString, 10);
	display_string(bufferString, 0);

	memset(bufferString, 0, sizeof(bufferString));

	change_position(240,70);     //load 2 call
    change_foreground(BLACK);
    change_background(YELLOW);
	ltoa(CALL_L2, bufferString, 10);
	display_string(bufferString, 0);

	memset(bufferString, 0, sizeof(bufferString));

	change_position(240,100);     //load 3 call
    change_foreground(BLACK);
    change_background(YELLOW);
	ltoa(CALL_L3, bufferString, 10);
	display_string(bufferString, 0);
	
	change_position(70,40);    //wind
	change_foreground(BLACK);
	change_background(YELLOW);
	wind_scaled = WIND_CAP * 100.0;
	snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)wind_scaled / 100, (int)wind_scaled % 100);
	display_string(bufferString, 0); // Display the entire string at once

	change_position(70,70);   //solar
	change_foreground(BLACK);
	change_background(YELLOW);
	solar_scaled = PV_CAP * 100.0;
	snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)solar_scaled / 100, (int)solar_scaled % 100);
	display_string(bufferString, 0); // Display the entire string at once

	change_position(120,130);   //busbar voltage
    change_foreground(BLACK);
    change_background(YELLOW);
    snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)BUSBAR_VOLTAGE, (int)BUSBAR_VOLTAGE);
    display_char(bufferString[0], 0); // Display the entire string at once
    display_char(bufferString[1], 0);
    display_char(bufferString[2], 0);
    display_char(bufferString[3], 0);
    display_char(bufferString[4], 0);

	change_position(120,160);   //busbar current
    change_foreground(BLACK);
    change_background(YELLOW);
	current_scaled = BUSBAR_CURRENT * 100.0;
    snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)current_scaled / 100, (int)current_scaled % 100);
    display_string(bufferString, 0); // Display the entire string at once
	
	change_position(230,160);  //busbar power
    change_foreground(BLACK);
    change_background(YELLOW);
	power = (BUSBAR_CURRENT * BUSBAR_VOLTAGE)/2.0;  //returns average power(as busbar current and voltage are AC)
	snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)power, (int)power);
	if(power>=100 && power<1000)
	{
		display_char(bufferString[0], 0); // Display the entire string at once
		display_char(bufferString[1], 0);
		display_char(bufferString[2], 0);
		display_char(bufferString[3], 0);
		display_char(bufferString[4], 0);
	}
	else
	{
		display_char(bufferString[0], 0); // Display the entire string at once
		display_char(bufferString[1], 0);
		display_char(bufferString[2], 0);
		display_char(bufferString[3], 0);	
		display_char(bufferString[4], 0);
		display_char(bufferString[5], 0);
	}
	
	// Update the last update time
    lastUpdateTime = currentTime;
}

// Iterates through all possible outcomes 
void optimal_choice(bool optimal_load_outcome[BITS])
{
	double mains_required[OUTCOMES]; 
    double incurred_cost[OUTCOMES];
    double wasted_current[OUTCOMES];
    double current_required[OUTCOMES];

    // Define weights & weight sum (3 bits -> 2^3 combinations)
    double weighted_sum[OUTCOMES]; 

    // Define array of all possible BITS sized outcomes 
    bool outcomes[OUTCOMES][BITS]; 

    // Define and initializes index of invalid combinations (1 = invalid)
    bool invalid_combs[OUTCOMES] = {0,0,0,0,0,0,0,0};

    // Generate all possible outcomes
    get_possible_outcomes(outcomes); 

    //instantiate Arrays
    instantiate_array(mains_required); 
    instantiate_array(incurred_cost); 
    instantiate_array(current_required);
    instantiate_array(wasted_current); 
    instantiate_array(weighted_sum); 

    // Calculates Minimizing terms for each valid outcome 
    for(i = 0; i < OUTCOMES; i++)
	{
        mains_required[i] = get_mains_required(outcomes[i]); 
        incurred_cost[i] = get_incurred_cost(outcomes[i]); 
        wasted_current[i] = get_wasted_current(outcomes[i]); 
        current_required[i] = get_current_required(outcomes[i]);
    }

    // Check charge & discharge condition for each combination
    // If 1 = charge / discharge , If = not charge / discharge
    // Charge cannot occur at the same time as discharge
    bool charge_select[OUTCOMES] = {0,0,0,0,0,0,0,0}; 
    bool discharge_select[OUTCOMES] = {0,0,0,0,0,0,0,0};
    charge_condition(charge_select, current_required, wasted_current);
    discharge_condition(charge_select, discharge_select, mains_required, incurred_cost);

    // Checks validity of combinations 
    validity_checker(outcomes, mains_required, invalid_combs);

    // Make invalid combinations have large numbers
    for (i = 0; i < OUTCOMES; i++)
	{
        if (invalid_combs[i])
		{
            mains_required[i] = __INT_MAX__/2;
            incurred_cost[i] = __INT_MAX__/2; 
            wasted_current[i] = __INT_MAX__/2; 
            current_required[i] = __INT_MAX__/2; 
        }
    }

    // Define normalized minimizing terms 
    double normalized_mains_required[OUTCOMES]; 
    double normalized_incurred_cost[OUTCOMES];
    double normalized_wasted_current[OUTCOMES];

    // Normalize valid minimizing terms 
    normalize(mains_required, outcomes, invalid_combs, normalized_mains_required); 
    normalize(incurred_cost, outcomes, invalid_combs, normalized_incurred_cost);
    normalize(wasted_current, outcomes, invalid_combs, normalized_wasted_current);

    // Weighted Sum Calculatiosn 
    for(i = 0; i < OUTCOMES; i++)
	{
        weighted_sum[i] = WEIGHT_MR*normalized_mains_required[i] + WEIGHT_IC*normalized_incurred_cost[i] + WEIGHT_WC*normalized_wasted_current[i]; 
    }

    // Find smallest / optimal Switch combination
    int opt_index = get_optimal_index(weighted_sum); 
    double opt_mains_required = mains_required[opt_index];

	// Check if battery should be charged (update battery every second)
	if (charge_select[opt_index] == 1) 
	{
		// Add 1/60A per second
		BATTERY_CURRENT_STORED += (1.0f / 60.0f) * timeElapsed; // 1/60A per second
		CHARGE_BATTERY = 1;
	} 
	else 
	{
		CHARGE_BATTERY = 0;
	}

	if (discharge_select[opt_index] == 1) 
	{
		// Subtract 1/60A per second
		BATTERY_CURRENT_STORED -= (1.0f / 60.0f) * timeElapsed; // 1/60A per second
		DISCHARGE_BATTERY = 1;
	} 
	else 
	{
		DISCHARGE_BATTERY = 0;
	}

    // Set outputs 
    MAINS_I = opt_mains_required;
	char bufferString[10];
		
	SW_L1 = outcomes[opt_index][0];
	SW_L2 = outcomes[opt_index][1];
	SW_L3 = outcomes[opt_index][2];
	
	/*if(MAINS_I + WIND_CAP + PV_CAP < BUSBAR_CURRENT)
	{
		double error = (MAINS_I + WIND_CAP + PV_CAP) - BUSBAR_CURRENT;
		
		MAINS_I = MAINS_I + (Kp*error);
	}
	
	if(MAINS_I > MAINS_MAX_CAPACITY)
	{
		MAINS_I = MAINS_MAX_CAPACITY;
	}
	
	else if(MAINS_I < 0)
	{
		MAINS_I = 0;
	}*/
	
	//set outputs based on algorithm
	return_PWM(MAINS_I);
	//_delay_ms(15);// Transient on the circuit adjustment
	sw_load1(SW_L1);
	sw_load2(SW_L2);
	sw_load3(SW_L3);
	set_battery_charge_pin(CHARGE_BATTERY);
	set_battery_discharge_pin(DISCHARGE_BATTERY);
	
	change_position(165,183);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("SWITCHES: ", 0); //displays current time
	
	change_position(70,100);
	change_foreground(BLACK);
	change_background(YELLOW);
	MAINS_I_SCALED = MAINS_I * 100;
	snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)MAINS_I_SCALED / 100, (int)MAINS_I_SCALED % 100);
	display_string(bufferString, 0);
	
	rectangle sw1 = {.left = 180, .right = 190, .top = 200, .bottom = 210};
	if(SW_L1 == 1)
		fill_rectangle(sw1, GREEN);
	else
		fill_rectangle(sw1, RED);
	
	change_position(182,212);
    change_foreground(BLACK);
    change_background(YELLOW);
	snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)SW_L1, (int)SW_L1);
	display_char(bufferString[0], 0);
	
	rectangle sw2 = {.left = 205, .right = 215, .top = 200, .bottom = 210};
	if(SW_L2 == 1)
		fill_rectangle(sw2, GREEN);
	else
		fill_rectangle(sw2, RED);
	
	change_position(207,212);
    change_foreground(BLACK);
    change_background(YELLOW);
	snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)SW_L2, (int)SW_L2);
	display_char(bufferString[0], 0);
	
	rectangle sw3 = {.left = 230, .right = 240, .top = 200, .bottom = 210};
	if(SW_L3 == 1)
		fill_rectangle(sw3, GREEN);
	else
		fill_rectangle(sw3, RED);
	
	change_position(232,212);
    change_foreground(BLACK);
    change_background(YELLOW);
	snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)SW_L3, (int)SW_L3);
	display_char(bufferString[0], 0);
	
	change_position(240,17);
    change_foreground(BLACK);
    change_background(YELLOW);
	BATTERY_CURRENT_STORED_SCALED = BATTERY_CURRENT_STORED * 100.0;
	snprintf(bufferString, sizeof(bufferString), "%d.%02d", (int)BATTERY_CURRENT_STORED_SCALED / 100, (int)BATTERY_CURRENT_STORED_SCALED % 100);
	display_char(bufferString[0], 1);
	display_char(bufferString[1], 1);
	display_char(bufferString[2], 1);
	display_char(bufferString[3], 1);

	if(CHARGE_BATTERY == 1 && DISCHARGE_BATTERY == 0)
	{
		rectangle sw3 = {.left = 293, .right = 313, .top = 17, .bottom = 27};
		fill_rectangle(sw3, GREEN);
		draw_charge_symbol();
	}
	
	else if(CHARGE_BATTERY == 0 && DISCHARGE_BATTERY == 1)
	{
		clear_charge_symbol();
		rectangle sw3 = {.left = 293, .right = 313, .top = 17, .bottom = 27};
		fill_rectangle(sw3, RED);
	}
	
	else
	{
		clear_charge_symbol();
		rectangle sw3 = {.left = 293, .right = 313, .top = 17, .bottom = 27};
		fill_rectangle(sw3, BLUE);
	}
    // Set the optimal load solution
    memcpy(optimal_load_outcome, outcomes[opt_index], sizeof(bool) * BITS);
}

int get_optimal_index(double weighted_sum[]){
    // Loops through all weighted sum for optimal choice (smallest sum)
    double smallest_sum = __INT_MAX__; 
    for(i = 0; i < OUTCOMES; i++){
        if(weighted_sum[i] < smallest_sum) {
            smallest_index = i; 
            smallest_sum = weighted_sum[i]; 
        }
    }
    return smallest_index; 
}

double get_wasted_current(const bool outcome[BITS]){
    double current_required = get_current_required(outcome); 

    // Situation when there is excess current > 1A and tell that 1A has been stored
    if (current_required < 0){
        return -1*(current_required); 
    }
    else {
        return 0; 
    }
}

// Calculates Cost of not turning a Load on when called for 
double get_incurred_cost(const bool outcome[BITS]){
    //change boolean expression depending on the scenario 
    double incurred_cost = (!outcome[0])*!((!outcome[0] & !outcome[1]) & (CALL_L1 & CALL_L2))*CALL_L1*COST_L1 + (!outcome[2])*CALL_L3*COST_L3 + (!outcome[1])*!((!outcome[0] & !outcome[1]) & (CALL_L1 & CALL_L2))*CALL_L2*COST_L2 + ((!outcome[0] & !outcome[1]) & (CALL_L1 & CALL_L2))*COST_L1_L2;
    return incurred_cost; 
}

// Calculates Mains Required 
double get_mains_required(const bool outcome[BITS]){
    double mains_required = get_current_required(outcome); 

    if (mains_required >= 0) return mains_required; 
    else return 0; 
}

// Calculates Current Required
double get_current_required(const bool outcome[BITS]){
    // Total required current - total renewable current available
    double current_required = outcome[0] * USAGE_L1 + outcome[1] * USAGE_L2 + outcome[2] * USAGE_L3 - (WIND_CAP + PV_CAP); 
    return current_required; 
}

// Check if possible to charge for a certain combination
void charge_condition(bool charge_select[], double current_required[], double wasted_current[]){ 
    // Situation when there is excess current > 1A and tell that 1A has been stored
    for(i = 0; i < OUTCOMES; i++){
        if ((current_required[i] < 0)&& !DISCHARGE_BATTERY){
            if(current_required[i] <= -1){
				wasted_current[i] -= BATTERY_PER_HOUR;
                charge_select[i] = 1; 
            }
            else {
                charge_select[i] = 0;  
            }
    }
    else charge_select[i] = 0; 
    }
}

// Check if possible to discharge for a certain combination
void discharge_condition(bool charge_select[], bool discharge_select[], double mains_required[], double incurred_cost[]){
    // Discharge Condition (CAREFUL TO NOT DISCHARGE MORE THAN CHARGED!)
    // If discharge from battery can subtract 1 A from required mains 
    // (Should not charge and discharge at same time to avoid incorrect information)
    for (i = 0; i < OUTCOMES; i++){
        if (((mains_required[i] >= MAINS_MAX_CAPACITY)||((mains_required[i] >= 1)&&(counter >= 1200000))) && (BATTERY_CURRENT_STORED >= 0.30) && !(charge_select[i])) {
        mains_required[i] = mains_required[i] - BATTERY_PER_HOUR; 
        if (mains_required[i] < 0) mains_required[i] = 0; 
        discharge_select[i] = 1; 
    }
    else discharge_select[i] = 0; 
    }
}

// Tells the algorithm which switch combinations are invalid (constraint equations)
void validity_checker(bool outcomes[][BITS], double mains_required[], bool invalid_combs[]){
    // Checks time, Operating Theatre cannot be used before 08:00 or after 22:00 
    for (i = 0; i < OUTCOMES; i++){
        if ((counter < 480000 || counter > 1320000) && outcomes[i][0]) {
            invalid_combs[i] = 1; 
        } else {
            // Check call and switch mismatch directly here
            bool parent[BITS] = {CALL_L1, CALL_L2, CALL_L3}; 
            bool child[BITS];
            for (j = 0; j < BITS; j++) {
                child[j] = outcomes[i][j];
            }
            for (j = 0; j < BITS; j++) {
                if (!parent[j] && child[j]) {
                    invalid_combs[i] = 1;
                    break;
                }
            }
        }
        
        // Checks if mains required > max main capacity
        if (fabs(mains_required[i]) > MAINS_MAX_CAPACITY) {
            invalid_combs[i] = 1; 
        }
    }
}

void normalize(double parent[], bool outcomes[][BITS], bool invalid_combs[], double child[]) {
    // Find the maximum element in the oldArray
    double max = 0;
    for (i = 0; i < OUTCOMES; i++) {
        if ((parent[i] > max)&&!invalid_combs[i]) {
            max = parent[i];
        }
    }

    // Normalize the elements by dividing each by the maximum
    for (i = 0; i < OUTCOMES; i++) {
        if (max == 0) child[i] = 0; 
        else child[i] = parent[i] / max;
    }
}

void get_possible_outcomes(bool expressions[][BITS]) {
     for (i = 0; i < OUTCOMES; i++) {
        for (j = 0; j < BITS; j++) {
            expressions[i][j] = (i >> j) & 1; // Extract each bit and store in the array
        }
    }
}

void instantiate_array(double array[OUTCOMES]){
    for(i = 0; i < OUTCOMES; i++) array[i] = 0; 
}

void clear_charge_symbol()
{
	rectangle clear1 = {.left = 299, .right = 308, .top = 8, .bottom = 34};
	fill_rectangle(clear1, YELLOW);
}

void draw_charge_symbol()
{
	//for inside of lightning bolt
	rectangle sw200 = {.left = 305, .right = 306, .top = 9, .bottom = 10};
	fill_rectangle(sw200, WHITE);
	rectangle sw201 = {.left = 305, .right = 306, .top = 10, .bottom = 11};
	fill_rectangle(sw201, WHITE);
	rectangle sw202 = {.left = 305, .right = 306, .top = 11, .bottom = 12};
	fill_rectangle(sw202, WHITE);
	rectangle sw203 = {.left = 305, .right = 306, .top = 12, .bottom = 13};
	fill_rectangle(sw203, WHITE);
	rectangle sw204 = {.left = 305, .right = 306, .top = 13, .bottom = 14};
	fill_rectangle(sw204, WHITE);
	rectangle sw205 = {.left = 304, .right = 305, .top = 12, .bottom = 13};
	fill_rectangle(sw205, WHITE);
	rectangle sw206 = {.left = 304, .right = 305, .top = 13, .bottom = 14};
	fill_rectangle(sw206, WHITE);
	rectangle sw207 = {.left = 304, .right = 305, .top = 14, .bottom = 15};
	fill_rectangle(sw207, WHITE);
	rectangle sw208 = {.left = 304, .right = 305, .top = 15, .bottom = 16};
	fill_rectangle(sw208, WHITE);
	rectangle sw209 = {.left = 303, .right = 304, .top = 14, .bottom = 15};
	fill_rectangle(sw209, WHITE);
	rectangle sw210 = {.left = 303, .right = 304, .top = 15, .bottom = 16};
	fill_rectangle(sw210, WHITE);
	rectangle sw211 = {.left = 303, .right = 304, .top = 16, .bottom = 17};
	fill_rectangle(sw211, WHITE);
	rectangle sw212 = {.left = 303, .right = 304, .top = 17, .bottom = 18};
	fill_rectangle(sw212, WHITE);
	rectangle sw213 = {.left = 303, .right = 304, .top = 18, .bottom = 19};
	fill_rectangle(sw213, WHITE);
	rectangle sw214 = {.left = 303, .right = 304, .top = 19, .bottom = 20};
	fill_rectangle(sw214, WHITE);
	rectangle sw215 = {.left = 302, .right = 303, .top = 16, .bottom = 17};
	fill_rectangle(sw215, WHITE);
	rectangle sw216 = {.left = 302, .right = 303, .top = 17, .bottom = 18};
	fill_rectangle(sw216, WHITE);
	rectangle sw217 = {.left = 302, .right = 303, .top = 18, .bottom = 19};
	fill_rectangle(sw217, WHITE);
	rectangle sw218 = {.left = 302, .right = 303, .top = 19, .bottom = 20};
	fill_rectangle(sw218, WHITE);
	rectangle sw219 = {.left = 301, .right = 302, .top = 18, .bottom = 19};
	fill_rectangle(sw219, WHITE);
	rectangle sw220 = {.left = 301, .right = 302, .top = 19, .bottom = 20};
	fill_rectangle(sw220, WHITE);
	//big rectangle in the middle
	rectangle sw221 = {.left = 300, .right = 308, .top = 20, .bottom = 22};
	fill_rectangle(sw221, WHITE);
	
	rectangle sw222 = {.left = 304, .right = 307, .top = 22, .bottom = 24};
	fill_rectangle(sw222, WHITE);
	rectangle sw223 = {.left = 304, .right = 306, .top = 24, .bottom = 26};
	fill_rectangle(sw223, WHITE);
	rectangle sw224 = {.left = 303, .right = 305, .top = 26, .bottom = 28};
	fill_rectangle(sw224, WHITE);
	rectangle sw225 = {.left = 302, .right = 304, .top = 28, .bottom = 30};
	fill_rectangle(sw225, WHITE);
	rectangle sw226 = {.left = 302, .right = 303, .top = 30, .bottom = 33};
	fill_rectangle(sw226, WHITE);
	
	//for border of lightning bolt
	rectangle sw3 = {.left = 305, .right = 306, .top = 8, .bottom = 9};
	fill_rectangle(sw3, BLACK);
	rectangle sw4 = {.left = 306, .right = 307, .top = 8, .bottom = 9};
	fill_rectangle(sw4, BLACK);
	rectangle sw5 = {.left = 306, .right = 307, .top = 9, .bottom = 10};
	fill_rectangle(sw5, BLACK);
	rectangle sw6 = {.left = 306, .right = 307, .top = 10, .bottom = 11};
	fill_rectangle(sw6, BLACK);
	rectangle sw7 = {.left = 306, .right = 307, .top = 11, .bottom = 12};
	fill_rectangle(sw7, BLACK);
	rectangle sw8 = {.left = 306, .right = 307, .top = 12, .bottom = 13};
	fill_rectangle(sw8, BLACK);
	rectangle sw9 = {.left = 306, .right = 307, .top = 13, .bottom = 14};
	fill_rectangle(sw9, BLACK);
	rectangle sw10 = {.left = 305, .right = 306, .top = 14, .bottom = 15};
	fill_rectangle(sw10, BLACK);
	rectangle sw11 = {.left = 305, .right = 306, .top = 15, .bottom = 16};
	fill_rectangle(sw11, BLACK);
	rectangle sw12 = {.left = 305, .right = 306, .top = 16, .bottom = 17};
	fill_rectangle(sw12, BLACK);
	rectangle sw13 = {.left = 304, .right = 305, .top = 16, .bottom = 17};
	fill_rectangle(sw13, BLACK);
	rectangle sw14 = {.left = 304, .right = 305, .top = 17, .bottom = 18};
	fill_rectangle(sw14, BLACK);
	rectangle sw15 = {.left = 304, .right = 305, .top = 18, .bottom = 19};
	fill_rectangle(sw15, BLACK);
	rectangle sw16 = {.left = 304, .right = 305, .top = 19, .bottom = 20};
	fill_rectangle(sw16, BLACK);
	rectangle sw17 = {.left = 304, .right = 308, .top = 19, .bottom = 20};
	fill_rectangle(sw17, BLACK);
	rectangle sw18 = {.left = 307, .right = 308, .top = 19, .bottom = 23};
	fill_rectangle(sw18, BLACK);
	rectangle sw19 = {.left = 306, .right = 307, .top = 23, .bottom = 25};
	fill_rectangle(sw19, BLACK);
	rectangle sw20 = {.left = 305, .right = 306, .top = 25, .bottom = 27};
	fill_rectangle(sw20, BLACK);
	rectangle sw21 = {.left = 304, .right = 305, .top = 27, .bottom = 29};
	fill_rectangle(sw21, BLACK);
	rectangle sw22 = {.left = 303, .right = 304, .top = 29, .bottom = 31};
	fill_rectangle(sw22, BLACK);
	rectangle sw23 = {.left = 302, .right = 303, .top = 31, .bottom = 33};
	fill_rectangle(sw23, BLACK);
	rectangle sw24 = {.left = 300, .right = 302, .top = 33, .bottom = 34};
	fill_rectangle(sw24, BLACK);
	rectangle sw25 = {.left = 300, .right = 301, .top = 28, .bottom = 33};
	fill_rectangle(sw25, BLACK);
	rectangle sw26 = {.left = 301, .right = 302, .top = 25, .bottom = 28};
	fill_rectangle(sw26, BLACK);
	rectangle sw27 = {.left = 302, .right = 303, .top = 22, .bottom = 26};
	fill_rectangle(sw27, BLACK);
	rectangle sw28 = {.left = 299, .right = 303, .top = 22, .bottom = 23};
	fill_rectangle(sw28, BLACK);
	rectangle sw29	= {.left = 299, .right = 300, .top = 19, .bottom = 23};
	fill_rectangle(sw29, BLACK);
	rectangle sw30 = {.left = 300, .right = 301, .top = 17, .bottom = 20};
	fill_rectangle(sw30, BLACK);
	rectangle sw31 = {.left = 301, .right = 302, .top = 15, .bottom = 18};
	fill_rectangle(sw31, BLACK);
	rectangle sw32 = {.left = 302, .right = 303, .top = 13, .bottom = 16};
	fill_rectangle(sw32, BLACK);
	rectangle sw33 = {.left = 303, .right = 304, .top = 11, .bottom = 14};
	fill_rectangle(sw33, BLACK);
	rectangle sw34 = {.left = 304, .right = 305, .top = 9, .bottom = 12};
	fill_rectangle(sw34, BLACK);
}

void setup_PWM()
{
	DDRD |= _BV(PD5); //Output at PD5 of PWM      //set pin as outputs
    TCCR1A |= _BV(COM1A1) | _BV(WGM10); //Fast PWM, top = 0x00FF
	TCCR1B |=  _BV(CS11) | _BV(WGM12);  //prescalar = 8(frequency = 5.86kHz)
    TIMSK1 = _BV(TOIE1) | _BV(OCIE1B); //overflow interrupt for timer 1 and compare match B interrupt
    OCR1A = 0;  //initially 0V output
	OCR1B = 0;  //top value
}

uint8_t return_PWM(float volts)
{
	//value of OCR1A(0-255) determines the output voltage from that pin which depends on the current which is determined from the controller
	duty_cycle = (int)((PWM_OUT(volts) * MAINS_MULTIPLY) - 0.0645);
	return duty_cycle;
}

void power_reduction()
{
	PRR0 |= _BV(PRTWI) | _BV(PRUSART1) | _BV(PRUSART0);  //shuts down I2C interface, USART1 and USART0 channel
}

void init_timer()
{
    TCCR0A = 0x00;

    TCCR0B = 0x00;

    TCCR0B |= _BV(CS00) | _BV(CS01); //prescalar = 64

    TIMSK0 |= _BV(TOIE0); //enable interrupt

    TCNT0 = 68;

    counter = 0; //initial value for counter
}

void display_current_time()
{
	char theString[10];
    change_foreground(BLACK);
    change_background(YELLOW);
	uint16_t currentTime = counter / 1000;
    uint16_t minutes = currentTime / 60;
    uint16_t seconds = currentTime % 60;
	if(counter % 500 < 40)
	{
		if(moving == 0)
		{
			moving = 1;
			clear_ghost_red(0);
			clear_ghost_green(0);
			clear_ghost_purple(0);
			clear_ghost_orange(0);
			clear_pacman(0);
			
			if(BATTERY_CURRENT_STORED < 0.5)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(5, 0);
				init_ghost_red(5);
				init_ghost_green(5);
				init_ghost_purple(5);
				init_ghost_orange(5);
			}
			
			else if(BATTERY_CURRENT_STORED > 0.5 && BATTERY_CURRENT_STORED <= 1.0)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(5, 28);
				init_ghost_green(5);
				init_ghost_purple(5);
				init_ghost_orange(5);
			}
			
			else if(BATTERY_CURRENT_STORED > 1.0 && BATTERY_CURRENT_STORED <= 1.5)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(5, 47);
				init_ghost_purple(5);
				init_ghost_orange(5);
			}
			
			else if(BATTERY_CURRENT_STORED > 1.5 && BATTERY_CURRENT_STORED <= 2)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(5, 68);
				init_ghost_orange(5);
			}
			
			else if(BATTERY_CURRENT_STORED > 2 && BATTERY_CURRENT_STORED <= 2.5)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(5, 87);
			}
			
			else
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(5, 87);
			}
		}
		else
		{
			moving = 0;
			clear_ghost_red(5);
			clear_ghost_green(5);
			clear_ghost_purple(5);
			clear_ghost_orange(5);
			clear_pacman(0);
			if(BATTERY_CURRENT_STORED < 0.5)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(0, 0);
				init_ghost_red(0);
				init_ghost_green(0);
				init_ghost_purple(0);
				init_ghost_orange(0);
			}
			
			else if(BATTERY_CURRENT_STORED > 0.5 && BATTERY_CURRENT_STORED <= 1.0)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(0, 28);
				init_ghost_green(0);
				init_ghost_purple(0);
				init_ghost_orange(0);
			}
			
			else if(BATTERY_CURRENT_STORED > 1.0 && BATTERY_CURRENT_STORED <= 1.5)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(0, 47);
				init_ghost_purple(0);
				init_ghost_orange(0);
			}
			
			else if(BATTERY_CURRENT_STORED > 1.5 && BATTERY_CURRENT_STORED <= 2)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(0, 68);
				init_ghost_orange(0);
			}
			
			else if(BATTERY_CURRENT_STORED > 2 && BATTERY_CURRENT_STORED <= 2.5)
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(0, 87);
			}
			
			else
			{
				clear_pacman(0);
				clear_pacman(28);
				clear_pacman(47);
				clear_pacman(68);
				clear_pacman(87);
				init_display_pacman(5, 87);
			}
		}
	}
    ltoa(minutes, theString, 10);
    strcat(theString, ":");
    if (seconds < 10) 
    {
        strcat(theString, "0");
    }
    ltoa(seconds, theString + strlen(theString), 10);
    change_position(230,130);
    display_string(theString, 0); //displays current time
    display_string(" ", 0);
    display_string(" ", 0);
    display_string(" ", 0);
}
