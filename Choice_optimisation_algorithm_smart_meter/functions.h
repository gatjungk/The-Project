#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define DATA_IN DDRA
#define PORT_IN PORTA

#define DATA_OUT DDRD
#define PORT_OUT PORTD

#define WIND_CAP_PIN PINA0
#define PV_CAP_PIN PINA1
#define BB_VOLTAGE_PIN PINA2
#define BB_CURRENT_PIN PINA3
#define LOAD1_CALL PINA4
#define LOAD2_CALL PINA5
#define LOAD3_CALL PINA6

#define PIN_LOAD1_CALL PA4
#define PIN_LOAD2_CALL PA5
#define PIN_LOAD3_CALL PA6

#define CHARGE PIND0
#define DISCHARGE PIND1
#define SWLOAD1 PIND2
#define SWLOAD2 PIND3
#define SWLOAD3 PIND4

// Max capacity of sources 
#define MAINS_MAX_CAPACITY   2.0            // Maximum current the mains can draw 
#define BATTERY_PER_HOUR     1.0            // Discharge / charge rate of the battery

#define ADC_VOLT(x) (x * (3.3/1024.0))
#define PWM_OUT(x) ((x/MAINS_MAX_CAPACITY) * 255.0)

// Load Current Usage (A) 
#define USAGE_L1        1.2             //Current drawn from load L1
#define USAGE_L2        2.0             //Current drawn from load L2
#define USAGE_L3        0.8             //Current drawn from load L3

// Scenario / profile specific parameters 
#define COST_L1         4               //Cost of not turning on Just L1 when requested
#define COST_L2         3               //Cost of not turning on Just L2 when requested
#define COST_L3         1               //Cost of not turning on Just L3 when requested
#define COST_L1_L2      10              //Cost of not turning on L1 & L2 when requested

// Define weights for each minimizing term
#define WEIGHT_MR       0.2             // Weights for Main Required
#define WEIGHT_IC       0.5             // Weights for Incurred Cost
#define WEIGHT_WC       0.3             // Weights for Wasted Current

// No. controllable variables 
#define BITS            3               //3 controllable load switch outputs
// No. of possible outcomes
#define OUTCOMES        1 << BITS       //2^3 possible ways to switch the loads

#define FILTER_ORDER 16 // # of taps

#define MAINS_MULTIPLY 0.97343
#define WIND_MULTIPLY 1.06
#define PV_MULTIPLY 1.06
#define BBV_MULTIPLY 0.873423
#define BBI_MULTIPLY 1.06658

void init_timer();
void init_display_pacman(uint8_t shift, uint8_t horizontal_shift);
void init_ghost_red(uint8_t shift);
void init_ghost_green(uint8_t shift);
void init_ghost_purple(uint8_t shift);
void init_ghost_orange(uint8_t shift);

void clear_pacman(uint8_t horizontal_shift);
void clear_ghost_red(uint8_t shift);
void clear_ghost_green(uint8_t shift);
void clear_ghost_purple(uint8_t shift);
void clear_ghost_orange(uint8_t shift);

void battery_display(uint8_t state);
void info_display_setup();
void display_current_time();
void start_counter();

void init_adc();
void init_digital();
uint16_t read_adc(uint8_t channelNum);
uint8_t return_digital(uint8_t pin_num);
void setup_PWM();
uint8_t return_PWM(float volts);
void init_FIR_coeff(void);
void init_adc_buffer(void);
double fir_filter(double new_reading, int BS);

//digital output 
void set_battery_charge_pin(uint8_t inp);
void set_battery_discharge_pin(uint8_t inp);
void sw_load1(uint8_t inp);
void sw_load2(uint8_t inp);
void sw_load3(uint8_t inp);

//digital input 
bool call_load1(void);
bool call_load2(void);
bool call_load3(void);

//analogue input
double return_windCap(void);
double return_PVCap(void);
double return_BBV(void);
double return_BBI(void);

//analogue output
void set_mains(float volts);

void algorithm();

// Minimizing Term Return Function
double get_current_required(const bool outcome[BITS]); 
double get_mains_required(const bool outcome[BITS]); 
double get_incurred_cost(const bool outcome[BITS]); 
double get_wasted_current(const bool outcome[BITS]); 

// Output Configuration Functions
void set_battery_charge(double current_required); 

// Utility Functions
void get_possible_outcomes(bool expressions[][BITS]);
void instantiate_array(double array[OUTCOMES]);
bool  is_invalid(bool child[]);
int get_optimal_index(double weighted_sum[]); 
void normalize(double parent[], bool outcomes[][BITS], bool invalid_combs[], double child[]);
void validity_checker(bool outcomes[][BITS], double mains_required[], bool invalid_combs[]);
void charge_condition(bool charge_select[], double current_required[], double wasted_current[]);
void discharge_condition(bool charge_select[], bool discharge_select[], double mains_required[], double incurred_cost[]);

// Optimisation Function (**)
void optimal_choice(bool optimal_load_outcome[BITS]); 

//For PID controller
float ReadSensor(float inp);