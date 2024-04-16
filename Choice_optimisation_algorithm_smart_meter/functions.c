#include "lcd.h"
#include "ili934x.h"
#include "font.h"
#include "avrlcd.h"
#include "functions.h"

int i, j, k, l;
volatile uint8_t charge_battery;
float voltage; 
int smallest_index;

// Uniform averaging coefficients
double fir_coefficients[FILTER_ORDER];

// Buffer to store the last N readings
double adc_buffer[4][FILTER_ORDER];

void init_FIR_coeff(void){
  	double maw = 1.0/FILTER_ORDER;
  	for(i = 0 ; i< FILTER_ORDER ; i++){
    	fir_coefficients[i] = maw;
  	}
}

void init_adc_buffer(void){
	for(i = 0 ; i< 4; i++){
		for(j =0 ; j<FILTER_ORDER; j++){
			adc_buffer[i][j] = 0;
		}
	}
}

// Function to add a new reading and get the filtered result
double fir_filter(double new_reading, int BS) {
    // Shift the old readings to make space for the new one
    for (i = FILTER_ORDER-1 ; i > 0; i--) {
        adc_buffer[BS][i] = adc_buffer[BS][i-1];
    }
    adc_buffer[BS][0] = new_reading; // Add the new reading at the start

    // Apply the FIR filter
    double filtered_value = 0;
    for (i = 0; i < FILTER_ORDER; i++) {
        filtered_value += adc_buffer[BS][i] * fir_coefficients[i];
    }

    return filtered_value;
}

void init_adc()
{
	//Initializing ADC Pins(inputs at A0, A1, A2 and A3 corresponding to wind cap, PV cap, busbar voltage and busbar current respectively)
	DATA_IN &= ~( _BV(WIND_CAP_PIN) | _BV(PV_CAP_PIN) | _BV(BB_VOLTAGE_PIN) | _BV(BB_CURRENT_PIN) ); //Sets pins as analogue inputs
	ADCSRA |= _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1); 								    //Pre-scaler = 64
}

void init_digital()
{
	//sets up the digital inputs on port A(A4, A5 and A6) and outputs on port D(D0, D1, D2, D3, D4)
	DATA_IN &= ~(_BV(LOAD1_CALL) | _BV(LOAD2_CALL) | _BV(LOAD3_CALL)); 					//configure as inputs	 
	PORT_IN &= ~(_BV(PIN_LOAD1_CALL) | _BV(PIN_LOAD2_CALL) | _BV(PIN_LOAD3_CALL)); 		//enable pullups				 
	DATA_OUT |= _BV(CHARGE) | _BV(DISCHARGE) | _BV(SWLOAD1) | _BV(SWLOAD2) | _BV(SWLOAD3); 
	PIND = 0;
}

uint16_t read_adc(uint8_t channelNum)
{
	//Function to read ADC Values
	ADMUX = channelNum; 			//channel initialisation
	ADCSRA |= _BV(ADSC); 			//start conversion
	while(ADCSRA & _BV(ADSC)){}; 	//wait until the ADC conversion is done.
	return ADC; 					//return the ADC data after ready.
}

uint8_t return_digital(uint8_t pin_num)
{
	if(pin_num == 0)
		return (PINA & (1 << WIND_CAP_PIN)) >> WIND_CAP_PIN;  //first 4 dont really matter as they are analogue
		
	else if(pin_num == 1)
		return (PINA & (1 << PV_CAP_PIN)) >> PV_CAP_PIN;
				
	else if(pin_num == 2)
		return (PINA & (1 << BB_VOLTAGE_PIN)) >> BB_VOLTAGE_PIN;
				
	else if(pin_num == 3)
		return (PINA & (1 << BB_CURRENT_PIN)) >> BB_CURRENT_PIN;
				
	else if(pin_num == 4)
		return (PINA & (1 << LOAD1_CALL)) >> LOAD1_CALL;
				
	else if(pin_num == 5)
		return (PINA & (1 << LOAD2_CALL)) >> LOAD2_CALL;
				
	else if(pin_num == 6)
		return (PINA & (1 << LOAD3_CALL)) >> LOAD3_CALL;
				
	else
		return 0;
}

//digital output
void set_battery_charge_pin(uint8_t inp)
{
	if(inp == 1)
	{
		PORT_OUT |= _BV(CHARGE);
	}
	
	else
	{
		PORT_OUT &= ~_BV(CHARGE);
	}
}

void set_battery_discharge_pin(uint8_t inp)
{
	if(inp == 1)
	{
		PORT_OUT |= _BV(DISCHARGE);
	}
	
	else
	{
		PORT_OUT &= ~_BV(DISCHARGE);
	}
}

void sw_load1(uint8_t inp)
{
	if(inp == 1)
	{
		PORT_OUT |= _BV(SWLOAD1);
	}
	
	else
	{
		PORT_OUT &= ~_BV(SWLOAD1);
	}
}

void sw_load2(uint8_t inp)
{
	if(inp == 1)
	{
		PORT_OUT |= _BV(SWLOAD2);
	}
	
	else
	{
		PORT_OUT &= ~_BV(SWLOAD2);
	}
}

void sw_load3(uint8_t inp)
{
	if(inp == 1)
	{
		PORT_OUT |= _BV(SWLOAD3);
	}
	
	else
	{
		PORT_OUT &= ~_BV(SWLOAD3);
	}
}

//digital input
bool call_load1(void)
{
	return !(return_digital(4));  //port 4 circuitry reverses logic
}

bool call_load2(void)
{
	return !(return_digital(5));
}

bool call_load3(void)
{
	return !(return_digital(6));
}

//analogue input(includes linear regression for testbed purposes)
double return_windCap(void)
{
	return (fir_filter(ADC_VOLT(read_adc(0)), 0) * (5.0/3.3)) * WIND_MULTIPLY; //to scale from 0-3.3 to 0-5(for display and algorithm);
}

double return_PVCap(void)
{
	return (fir_filter(ADC_VOLT(read_adc(1)), 1) * (5.0/3.3)) * PV_MULTIPLY; //to scale from 0-3.3 to 0-5(for display and algorithm);
}

double return_BBV(void)
{
	return (fir_filter(ADC_VOLT(read_adc(2)), 2) * (4.0/3.3) * 100.0) * BBV_MULTIPLY; //to scale from 0-3.3 to 0-400(for display and algorithm);
}

double return_BBI(void)
{
	return (fir_filter(ADC_VOLT(read_adc(3)), 3) * (10.0/3.3)) * BBI_MULTIPLY; //to scale from 0-3.3 to 0-10(for display and algorithm);
}

void set_mains(float volts)
{
	
}

void info_display_setup()
{
	rectangle line70 = {.left = 154, .right = 155, .top = 0, .bottom = 240}; //line down the middle
	fill_rectangle(line70, BLACK);
	
	change_position(6,40);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Wind(A): ", 0); //displays wind capacity
	
	rectangle line1 = {.left = 0, .right = 160, .top = 56, .bottom = 57};
	fill_rectangle(line1, BLACK);
	
	change_position(6,70);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Solar(A): ", 0); //displays solar capacity
	
	rectangle line2 = {.left = 0, .right = 160, .top = 86, .bottom = 87};
	fill_rectangle(line2, BLACK);
	
	change_position(6,100);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Mains(A): ", 0); //displays mains capacity
	
	rectangle line3 = {.left = 0, .right = 160, .top = 116, .bottom = 117};
	fill_rectangle(line3, BLACK);
	
	change_position(6,130);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Busbar Voltage(V): ", 0); //displays busbar voltage
	
	rectangle line4 = {.left = 0, .right = 160, .top = 146, .bottom = 147};
	fill_rectangle(line4, BLACK);
	
	change_position(6,160);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Busbar Current(A): ", 0); //displays busbar current
	
	rectangle line10 = {.left = 0, .right = 160, .top = 176, .bottom = 177};
	fill_rectangle(line10, BLACK);
	
	change_position(166,40);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Load Call 1: ", 0); //displays load 1
	
	rectangle line5 = {.left = 160, .right = 320, .top = 56, .bottom = 57};
	fill_rectangle(line5, BLACK);
	
	change_position(166,70);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Load Call 2: ", 0); //displays load 2
	
	rectangle line6 = {.left = 160, .right = 320, .top = 86, .bottom = 87};
	fill_rectangle(line6, BLACK);
	
	change_position(166,100);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Load Call 3: ", 0); //displays load 3
	
	rectangle line7 = {.left = 160, .right = 320, .top = 116, .bottom = 117};
	fill_rectangle(line7, BLACK);
	
	rectangle line8 = {.left = 160, .right = 320, .top = 146, .bottom = 147};
	fill_rectangle(line8, BLACK);
	
	change_position(166,130);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Time(s): ", 0); //displays current time
	
	rectangle line9 = {.left = 160, .right = 320, .top = 176, .bottom = 177};
	fill_rectangle(line9, BLACK);
	
	change_position(166,160);
    change_foreground(BLACK);
    change_background(YELLOW);
	display_string("Power(W): ", 0); //displays power
}

void clear_pacman(uint8_t horizontal_shift)
{
	rectangle bar201 = {.left = 2+horizontal_shift, .right = 25+horizontal_shift, .top = 192, .bottom = 214};
	fill_rectangle(bar201, YELLOW); //to delete
}


void init_display_pacman(uint8_t shift, uint8_t horizontal_shift)
{
	for(i = 0; i<6; i++)
	{
		//_delay_ms(100);
		rectangle back = {.left = 2+horizontal_shift, .right = 3+horizontal_shift, .top = 200, .bottom = 200+(i+1)};
		fill_rectangle(back, BLACK);
		/*rectangle back2 = {.left = 3, .right = 4, .top = 200, .bottom = 200+(i+1)};
		fill_rectangle(back2, YELLOW);*/
		rectangle top1 = {.left = 11+horizontal_shift, .right = 11+(i+1)+horizontal_shift, .top = 192, .bottom = 193};
		fill_rectangle(top1, BLACK);
		rectangle bottom1 = {.left = 11+horizontal_shift, .right = 11+(i+1)+horizontal_shift, .top = 213, .bottom = 214};
		fill_rectangle(bottom1, BLACK);
	}
	
	for(j = 0;j<2;j++)
	{
		//_delay_ms(100);
		rectangle back3 = {.left = 3+horizontal_shift, .right = 4+horizontal_shift, .top = 198, .bottom = 198+(j+1)};
		fill_rectangle(back3, BLACK);
		rectangle back4 = {.left = 4+horizontal_shift, .right = 5+horizontal_shift, .top = 196, .bottom = 196+(j+1)};
		fill_rectangle(back4, BLACK);
		rectangle back5 = {.left = 3+horizontal_shift, .right = 4+horizontal_shift, .top = 207, .bottom = 207+(j+1)};
		fill_rectangle(back5, BLACK);
		rectangle back6 = {.left = 4+horizontal_shift, .right = 5+horizontal_shift, .top = 209, .bottom = 209+(j+1)};
		fill_rectangle(back6, BLACK);
		
		rectangle back9 = {.left = 6+horizontal_shift, .right = 6+(j+1)+horizontal_shift, .top = 194, .bottom = 195};
		fill_rectangle(back9, BLACK);
		
		rectangle back10 = {.left = 6+horizontal_shift, .right = 6+(j+1)+horizontal_shift, .top = 211, .bottom = 212};
		fill_rectangle(back10, BLACK);
		
		rectangle back11 = {.left = 8+horizontal_shift, .right = 8+(j+1)+horizontal_shift, .top = 193, .bottom = 194};
		fill_rectangle(back11, BLACK);
		
		rectangle back12 = {.left = 8+horizontal_shift, .right = 8+(j+1)+horizontal_shift, .top = 212, .bottom = 213};
		fill_rectangle(back12, BLACK);
		
		rectangle front1 = {.left = 17+horizontal_shift, .right = 17+(j+1)+horizontal_shift, .top = 193, .bottom = 194};
		fill_rectangle(front1, BLACK);
		
		rectangle front2 = {.left = 19+horizontal_shift, .right = 19+(j+1)+horizontal_shift, .top = 194, .bottom = 195};
		fill_rectangle(front2, BLACK);
		
		rectangle front3 = {.left = 17+horizontal_shift, .right = 17+(j+1)+horizontal_shift, .top = 212, .bottom = 213};
		fill_rectangle(front3, BLACK);
		
		rectangle front4 = {.left = 19+horizontal_shift, .right = 19+(j+1)+horizontal_shift, .top = 211, .bottom = 212};
		fill_rectangle(front4, BLACK);
		
		
		rectangle front7 = {.left = 22+horizontal_shift, .right = 23+horizontal_shift, .top = 196, .bottom = 196+(j+1)};
		fill_rectangle(front7, BLACK);
		
		rectangle front8 = {.left = 23+horizontal_shift, .right = 24+horizontal_shift, .top = 198, .bottom = 198+(j+1)};
		fill_rectangle(front8, BLACK);
		
		if(shift != 0)
		{
			rectangle front9 = {.left = 24+horizontal_shift, .right = 25+horizontal_shift, .top = 200, .bottom = 200+(j+1)};
			fill_rectangle(front9, BLACK);
		
			rectangle front10 = {.left = 24+horizontal_shift, .right = 25+horizontal_shift, .top = 202, .bottom = 202+(j+1)};
			fill_rectangle(front10, BLACK);
			
			rectangle front11 = {.left = 23+horizontal_shift, .right = 24+horizontal_shift, .top = 205, .bottom = 205+(j+1)};
			fill_rectangle(front11, BLACK);
		
			rectangle front12 = {.left = 23+horizontal_shift, .right = 24+horizontal_shift, .top = 207, .bottom = 207+(j+1)};
			fill_rectangle(front12, BLACK);
		}
		
		if(shift != 0)
		{
			rectangle mouth1 = {.left = 21+horizontal_shift, .right = 21+(j+1)+horizontal_shift, .top = 200+(shift-1), .bottom = 201+(shift-1)};
			fill_rectangle(mouth1, BLACK);
			
			rectangle mouth2 = {.left = 19+horizontal_shift, .right = 19+(j+1)+horizontal_shift, .top = 201+(shift-2), .bottom = 202+(shift-2)};
			fill_rectangle(mouth2, BLACK);
			
			rectangle mouth3 = {.left = 17+horizontal_shift, .right = 17+(j+1)+horizontal_shift, .top = 202+(shift-3), .bottom = 203+(shift-3)};
			fill_rectangle(mouth3, BLACK);
			
			rectangle mouth4 = {.left = 15+horizontal_shift, .right = 15+(j+1)+horizontal_shift, .top = 203+(shift-4), .bottom = 204+(shift-4)};
			fill_rectangle(mouth4, BLACK);
			
			rectangle mouth5 = {.left = 13+horizontal_shift, .right = 13+(j+1)+horizontal_shift, .top = 204+(shift-5), .bottom = 205+(shift-5)};
			fill_rectangle(mouth5, BLACK);
			
			rectangle mouth6 = {.left = 14+horizontal_shift, .right = 14+(j+1)+horizontal_shift, .top = 205-(shift-4), .bottom = 206-(shift-4)};
			fill_rectangle(mouth6, BLACK);
			
			rectangle mouth7 = {.left = 16+horizontal_shift, .right = 16+(j+1)+horizontal_shift, .top = 206-(shift-3), .bottom = 207-(shift-3)};
			fill_rectangle(mouth7, BLACK);
			
			rectangle mouth8 = {.left = 18+horizontal_shift, .right = 18+(j+1)+horizontal_shift, .top = 207-(shift-2), .bottom = 208-(shift-2)};
			fill_rectangle(mouth8, BLACK);
			
			rectangle mouth9 = {.left = 20+horizontal_shift, .right = 20+(j+1)+horizontal_shift, .top = 208-(shift-1), .bottom = 209-(shift-1)};
			fill_rectangle(mouth9, BLACK);
		}
		
		else
		{
			rectangle mouth1 = {.left = 21+horizontal_shift, .right = 21+(j+1)+horizontal_shift, .top = 200, .bottom = 201};
			fill_rectangle(mouth1, BLACK);
			
			rectangle mouth2 = {.left = 19+horizontal_shift, .right = 19+(j+1)+horizontal_shift, .top = 201, .bottom = 202};
			fill_rectangle(mouth2, BLACK);
			
			rectangle mouth3 = {.left = 17+horizontal_shift, .right = 17+(j+1)+horizontal_shift, .top = 202, .bottom = 203};
			fill_rectangle(mouth3, BLACK);
			
			rectangle mouth4 = {.left = 15+horizontal_shift, .right = 15+(j+1)+horizontal_shift, .top = 203, .bottom = 204};
			fill_rectangle(mouth4, BLACK);
			
			rectangle mouth5 = {.left = 13+horizontal_shift, .right = 13+(j+1)+horizontal_shift, .top = 204, .bottom = 205};
			fill_rectangle(mouth5, BLACK);
			
			rectangle mouth6 = {.left = 14+horizontal_shift, .right = 14+(j+1)+horizontal_shift, .top = 205, .bottom = 206};
			fill_rectangle(mouth6, BLACK);
			
			rectangle mouth7 = {.left = 16+horizontal_shift, .right = 16+(j+1)+horizontal_shift, .top = 206, .bottom = 207};
			fill_rectangle(mouth7, BLACK);
			
			rectangle mouth8 = {.left = 18+horizontal_shift, .right = 18+(j+1)+horizontal_shift, .top = 207, .bottom = 208};
			fill_rectangle(mouth8, BLACK);
			
			rectangle mouth9 = {.left = 20+horizontal_shift, .right = 20+(j+1)+horizontal_shift, .top = 208, .bottom = 209};
			fill_rectangle(mouth9, BLACK);
		}
		
		rectangle eye1 = {.left = 14+horizontal_shift, .right = 14+(j+1)+horizontal_shift, .top = 200, .bottom = 201};
		fill_rectangle(eye1, BLACK);
		rectangle eye2 = {.left = 13+horizontal_shift, .right = 13+(j+1)+horizontal_shift, .top = 199, .bottom = 200};
		fill_rectangle(eye2, BLACK);
		rectangle eye3 = {.left = 15+horizontal_shift, .right = 15+(j+1)+horizontal_shift, .top = 199, .bottom = 200};
		fill_rectangle(eye3, BLACK);
		rectangle eye4 = {.left = 13+horizontal_shift, .right = 13+(j+1)+horizontal_shift, .top = 198, .bottom = 199};
		fill_rectangle(eye4, BLACK);
		rectangle eye5 = {.left = 15+horizontal_shift, .right = 15+(j+1)+horizontal_shift, .top = 198, .bottom = 199};
		fill_rectangle(eye5, BLACK);
		rectangle eye6 = {.left = 13+horizontal_shift, .right = 13+(j+1)+horizontal_shift, .top = 197, .bottom = 198};
		fill_rectangle(eye6, BLACK);
		rectangle eye7 = {.left = 15+horizontal_shift, .right = 15+(j+1)+horizontal_shift, .top = 197, .bottom = 198};
		fill_rectangle(eye7, WHITE);
	}
	
	for(k = 0;k<1;k++)
	{
		//_delay_ms(100);
		rectangle back7 = {.left = 5+horizontal_shift, .right = 6+horizontal_shift, .top = 195, .bottom = 195+(k+1)};
		fill_rectangle(back7, BLACK);
		rectangle back8 = {.left = 5+horizontal_shift, .right = 6+horizontal_shift, .top = 210, .bottom = 210+(k+1)};
		fill_rectangle(back8, BLACK);
		
		rectangle front5 = {.left = 21+horizontal_shift, .right = 22+horizontal_shift, .top = 195, .bottom = 195+(k+1)};
		fill_rectangle(front5, BLACK);
		rectangle front6 = {.left = 21+horizontal_shift, .right = 22+horizontal_shift, .top = 210, .bottom = 210+(k+1)};
		fill_rectangle(front6, BLACK);
		
		rectangle front9 = {.left = 22+horizontal_shift, .right = 23+horizontal_shift, .top = 209, .bottom = 209+(k+1)};
		fill_rectangle(front9, BLACK);
		
		rectangle eye8 = {.left = 14+horizontal_shift, .right = 14+(k+1)+horizontal_shift, .top = 196, .bottom = 197};
		fill_rectangle(eye8, BLACK);
		rectangle eye9 = {.left = 15+horizontal_shift, .right = 15+(k+1)+horizontal_shift, .top = 196, .bottom = 197};
		fill_rectangle(eye9, WHITE);
	}
}

void clear_ghost_red(uint8_t shift)
{
	rectangle bar200 = {.left = 35, .right = 49, .top = 199-shift, .bottom = 212-shift};
	fill_rectangle(bar200, YELLOW); //to delete
}

void init_ghost_red(uint8_t shift)
{
	rectangle bar1 = {.left = 35, .right = 36, .top = 205-shift, .bottom = 211-shift};
	fill_rectangle(bar1, RED);
	rectangle bar2 = {.left = 36, .right = 37, .top = 202-shift, .bottom = 212-shift};
	fill_rectangle(bar2, RED);
	rectangle bar3 = {.left = 37, .right = 38, .top = 201-shift, .bottom = 212-shift};
	fill_rectangle(bar3, RED);
	
	rectangle bar4 = {.left = 38, .right = 39, .top = 200-shift, .bottom = 203-shift};
	fill_rectangle(bar4, RED);
	rectangle bar5 = {.left = 38, .right = 39, .top = 203-shift, .bottom = 207-shift};
	fill_rectangle(bar5, WHITE);
	rectangle bar6 = {.left = 38, .right = 39, .top = 207-shift, .bottom = 211-shift};
	fill_rectangle(bar6, RED);
	
	rectangle bar7 = {.left = 39, .right = 40, .top = 200-shift, .bottom = 202-shift};
	fill_rectangle(bar7, RED);
	rectangle bar8 = {.left = 39, .right = 40, .top = 202-shift, .bottom = 208-shift};
	fill_rectangle(bar8, WHITE);
	rectangle bar9 = {.left = 39, .right = 40, .top = 208-shift, .bottom = 210-shift};
	fill_rectangle(bar9, RED);
	
	rectangle bar10 = {.left = 40, .right = 41, .top = 199-shift, .bottom = 202-shift};
	fill_rectangle(bar10, RED);
	rectangle bar11 = {.left = 40, .right = 41, .top = 202-shift, .bottom = 204-shift};
	fill_rectangle(bar11, WHITE);
	rectangle bar12 = {.left = 40, .right = 41, .top = 204-shift, .bottom = 206-shift};
	fill_rectangle(bar12, GREY);
	rectangle bar13 = {.left = 40, .right = 41, .top = 206-shift, .bottom = 208-shift};
	fill_rectangle(bar13, WHITE);
	rectangle bar14 = {.left = 40, .right = 41, .top = 208-shift, .bottom = 211-shift};
	fill_rectangle(bar14, RED);
	
	rectangle bar15 = {.left = 41, .right = 42, .top = 199-shift, .bottom = 203-shift};
	fill_rectangle(bar15, RED);
	rectangle bar16 = {.left = 41, .right = 42, .top = 203-shift, .bottom = 204-shift};
	fill_rectangle(bar16, WHITE);
	rectangle bar17 = {.left = 41, .right = 42, .top = 204-shift, .bottom = 206-shift};
	fill_rectangle(bar17, GREY);
	rectangle bar18 = {.left = 41, .right = 42, .top = 206-shift, .bottom = 212-shift};
	fill_rectangle(bar18, RED);
	
	rectangle bar19 = {.left = 42, .right = 43, .top = 199-shift, .bottom = 212-shift};
	fill_rectangle(bar19, RED);
	
	rectangle bar20 = {.left = 43, .right = 44, .top = 199-shift, .bottom = 211-shift};
	fill_rectangle(bar20, RED);
	
	rectangle bar21 = {.left = 44, .right = 45, .top = 200-shift, .bottom = 203-shift};
	fill_rectangle(bar21, RED);
	rectangle bar22 = {.left = 44, .right = 45, .top = 203-shift, .bottom = 207-shift};
	fill_rectangle(bar22, WHITE);
	rectangle bar23 = {.left = 44, .right = 45, .top = 207-shift, .bottom = 210-shift};
	fill_rectangle(bar23, RED);
	
	rectangle bar24 = {.left = 45, .right = 46, .top = 200-shift, .bottom = 202-shift};
	fill_rectangle(bar24, RED);
	rectangle bar25 = {.left = 45, .right = 46, .top = 202-shift, .bottom = 208-shift};
	fill_rectangle(bar25, WHITE);
	rectangle bar26 = {.left = 45, .right = 46, .top = 208-shift, .bottom = 211-shift};
	fill_rectangle(bar26, RED);
	
	rectangle bar27 = {.left = 46, .right = 47, .top = 201-shift, .bottom = 202-shift};
	fill_rectangle(bar27, RED);
	rectangle bar28 = {.left = 46, .right = 47, .top = 202-shift, .bottom = 204-shift};
	fill_rectangle(bar28, WHITE);
	rectangle bar29 = {.left = 46, .right = 47, .top = 204-shift, .bottom = 206-shift};
	fill_rectangle(bar29, GREY);
	rectangle bar30 = {.left = 46, .right = 47, .top = 206-shift, .bottom = 208-shift};
	fill_rectangle(bar30, WHITE);
	rectangle bar31 = {.left = 46, .right = 47, .top = 208-shift, .bottom = 212-shift};
	fill_rectangle(bar31, RED);
	
	rectangle bar32 = {.left = 47, .right = 48, .top = 202-shift, .bottom = 203-shift};
	fill_rectangle(bar32, RED);
	rectangle bar33 = {.left = 47, .right = 48, .top = 203-shift, .bottom = 204-shift};
	fill_rectangle(bar33, WHITE);
	rectangle bar34 = {.left = 47, .right = 48, .top = 204-shift, .bottom = 206-shift};
	fill_rectangle(bar34, GREY);
	rectangle bar35 = {.left = 47, .right = 48, .top = 206-shift, .bottom = 212-shift};
	fill_rectangle(bar35, RED);
	
	rectangle bar36 = {.left = 48, .right = 49, .top = 203-shift, .bottom = 211-shift};
	fill_rectangle(bar36, RED);
	
}

void clear_ghost_green(uint8_t shift)
{
	rectangle bar200 = {.left = 55, .right = 69, .top = 197+shift, .bottom = 210+shift};
	fill_rectangle(bar200, YELLOW); //to delete
}

void init_ghost_green(uint8_t shift)
{
	rectangle bar1 = {.left = 55, .right = 56, .top = 203+shift, .bottom = 209+shift};
	fill_rectangle(bar1, GREEN);
	rectangle bar2 = {.left = 56, .right = 57, .top = 200+shift, .bottom = 210+shift};
	fill_rectangle(bar2, GREEN);
	rectangle bar3 = {.left = 57, .right = 58, .top = 199+shift, .bottom = 210+shift};
	fill_rectangle(bar3, GREEN);
	
	rectangle bar4 = {.left = 58, .right = 59, .top = 198+shift, .bottom = 201+shift};
	fill_rectangle(bar4, GREEN);
	rectangle bar5 = {.left = 58, .right = 59, .top = 201+shift, .bottom = 205+shift};
	fill_rectangle(bar5, WHITE);
	rectangle bar6 = {.left = 58, .right = 59, .top = 205+shift, .bottom = 209+shift};
	fill_rectangle(bar6, GREEN);
	
	rectangle bar7 = {.left = 59, .right = 60, .top = 198+shift, .bottom = 200+shift};
	fill_rectangle(bar7, GREEN);
	rectangle bar8 = {.left = 59, .right = 60, .top = 200+shift, .bottom = 206+shift};
	fill_rectangle(bar8, WHITE);
	rectangle bar9 = {.left = 59, .right = 60, .top = 206+shift, .bottom = 208+shift};
	fill_rectangle(bar9, GREEN);
	
	rectangle bar10 = {.left = 60, .right = 61, .top = 197+shift, .bottom = 200+shift};
	fill_rectangle(bar10, GREEN);
	rectangle bar11 = {.left = 60, .right = 61, .top = 200+shift, .bottom = 202+shift};
	fill_rectangle(bar11, WHITE);
	rectangle bar12 = {.left = 60, .right = 61, .top = 202+shift, .bottom = 204+shift};
	fill_rectangle(bar12, GREY);
	rectangle bar13 = {.left = 60, .right = 61, .top = 204+shift, .bottom = 206+shift};
	fill_rectangle(bar13, WHITE);
	rectangle bar14 = {.left = 60, .right = 61, .top = 206+shift, .bottom = 209+shift};
	fill_rectangle(bar14, GREEN);
	
	rectangle bar15 = {.left = 61, .right = 62, .top = 197+shift, .bottom = 201+shift};
	fill_rectangle(bar15, GREEN);
	rectangle bar16 = {.left = 61, .right = 62, .top = 201+shift, .bottom = 202+shift};
	fill_rectangle(bar16, WHITE);
	rectangle bar17 = {.left = 61, .right = 62, .top = 202+shift, .bottom = 204+shift};
	fill_rectangle(bar17, GREY);
	rectangle bar18 = {.left = 61, .right = 62, .top = 204+shift, .bottom = 210+shift};
	fill_rectangle(bar18, GREEN);
	
	rectangle bar19 = {.left = 62, .right = 63, .top = 197+shift, .bottom = 210+shift};
	fill_rectangle(bar19, GREEN);
	
	rectangle bar20 = {.left = 63, .right = 64, .top = 197+shift, .bottom = 209+shift};
	fill_rectangle(bar20, GREEN);
	
	rectangle bar21 = {.left = 64, .right = 65, .top = 198+shift, .bottom = 201+shift};
	fill_rectangle(bar21, GREEN);
	rectangle bar22 = {.left = 64, .right = 65, .top = 201+shift, .bottom = 205+shift};
	fill_rectangle(bar22, WHITE);
	rectangle bar23 = {.left = 64, .right = 65, .top = 205+shift, .bottom = 208+shift};
	fill_rectangle(bar23, GREEN);
	
	rectangle bar24 = {.left = 65, .right = 66, .top = 198+shift, .bottom = 200+shift};
	fill_rectangle(bar24, GREEN);
	rectangle bar25 = {.left = 65, .right = 66, .top = 200+shift, .bottom = 206+shift};
	fill_rectangle(bar25, WHITE);
	rectangle bar26 = {.left = 65, .right = 66, .top = 206+shift, .bottom = 209+shift};
	fill_rectangle(bar26, GREEN);
	
	rectangle bar27 = {.left = 66, .right = 67, .top = 199+shift, .bottom = 200+shift};
	fill_rectangle(bar27, GREEN);
	rectangle bar28 = {.left = 66, .right = 67, .top = 200+shift, .bottom = 202+shift};
	fill_rectangle(bar28, WHITE);
	rectangle bar29 = {.left = 66, .right = 67, .top = 202+shift, .bottom = 204+shift};
	fill_rectangle(bar29, GREY);
	rectangle bar30 = {.left = 66, .right = 67, .top = 204+shift, .bottom = 206+shift};
	fill_rectangle(bar30, WHITE);
	rectangle bar31 = {.left = 66, .right = 67, .top = 206+shift, .bottom = 210+shift};
	fill_rectangle(bar31, GREEN);
	
	rectangle bar32 = {.left = 67, .right = 68, .top = 200+shift, .bottom = 201+shift};
	fill_rectangle(bar32, GREEN);
	rectangle bar33 = {.left = 67, .right = 68, .top = 201+shift, .bottom = 203+shift};
	fill_rectangle(bar33, WHITE);
	rectangle bar34 = {.left = 67, .right = 68, .top = 202+shift, .bottom = 205+shift};
	fill_rectangle(bar34, GREY);
	rectangle bar35 = {.left = 67, .right = 68, .top = 204+shift, .bottom = 210+shift};
	fill_rectangle(bar35, GREEN);
	
	rectangle bar36 = {.left = 68, .right = 69, .top = 201+shift, .bottom = 209+shift};
	fill_rectangle(bar36, GREEN);
	
}

void clear_ghost_purple(uint8_t shift)
{
	rectangle bar200 = {.left = 75, .right = 89, .top = 201-shift, .bottom = 214-shift};
	fill_rectangle(bar200, YELLOW); //to delete
}

void init_ghost_purple(uint8_t shift)
{
	rectangle bar1 = {.left = 75, .right = 76, .top = 207-shift, .bottom = 213-shift};
	fill_rectangle(bar1, BLUE);
	rectangle bar2 = {.left = 76, .right = 77, .top = 204-shift, .bottom = 214-shift};
	fill_rectangle(bar2, BLUE);
	rectangle bar3 = {.left = 77, .right = 78, .top = 203-shift, .bottom = 214-shift};
	fill_rectangle(bar3, BLUE);
	
	rectangle bar4 = {.left = 78, .right = 79, .top = 202-shift, .bottom = 205-shift};
	fill_rectangle(bar4, BLUE);
	rectangle bar5 = {.left = 78, .right = 79, .top = 205-shift, .bottom = 209-shift};
	fill_rectangle(bar5, WHITE);
	rectangle bar6 = {.left = 78, .right = 79, .top = 209-shift, .bottom = 213-shift};
	fill_rectangle(bar6, BLUE);
	
	rectangle bar7 = {.left = 79, .right = 80, .top = 202-shift, .bottom = 204-shift};
	fill_rectangle(bar7, BLUE);
	rectangle bar8 = {.left = 79, .right = 80, .top = 204-shift, .bottom = 210-shift};
	fill_rectangle(bar8, WHITE);
	rectangle bar9 = {.left = 79, .right = 80, .top = 210-shift, .bottom = 212-shift};
	fill_rectangle(bar9, BLUE);
	
	rectangle bar10 = {.left = 80, .right = 81, .top = 203-shift, .bottom = 204-shift};
	fill_rectangle(bar10, BLUE);
	rectangle bar11 = {.left = 80, .right = 81, .top = 204-shift, .bottom = 206-shift};
	fill_rectangle(bar11, WHITE);
	rectangle bar12 = {.left = 80, .right = 81, .top = 206-shift, .bottom = 208-shift};
	fill_rectangle(bar12, GREY);
	rectangle bar13 = {.left = 80, .right = 81, .top = 208-shift, .bottom = 210-shift};
	fill_rectangle(bar13, WHITE);
	rectangle bar14 = {.left = 80, .right = 81, .top = 210-shift, .bottom = 213-shift};
	fill_rectangle(bar14, BLUE);
	
	rectangle bar15 = {.left = 81, .right = 82, .top = 201-shift, .bottom = 205-shift};
	fill_rectangle(bar15, BLUE);
	rectangle bar16 = {.left = 81, .right = 82, .top = 205-shift, .bottom = 206-shift};
	fill_rectangle(bar16, WHITE);
	rectangle bar17 = {.left = 81, .right = 82, .top = 206-shift, .bottom = 208-shift};
	fill_rectangle(bar17, GREY);
	rectangle bar18 = {.left = 81, .right = 82, .top = 208-shift, .bottom = 214-shift};
	fill_rectangle(bar18, BLUE);
	
	rectangle bar19 = {.left = 82, .right = 83, .top = 201-shift, .bottom = 214-shift};
	fill_rectangle(bar19, BLUE);
	
	rectangle bar20 = {.left = 83, .right = 84, .top = 201-shift, .bottom = 213-shift};
	fill_rectangle(bar20, BLUE);
	
	rectangle bar21 = {.left = 84, .right = 85, .top = 202-shift, .bottom = 205-shift};
	fill_rectangle(bar21, BLUE);
	rectangle bar22 = {.left = 84, .right = 85, .top = 205-shift, .bottom = 209-shift};
	fill_rectangle(bar22, WHITE);
	rectangle bar23 = {.left = 84, .right = 85, .top = 209-shift, .bottom = 212-shift};
	fill_rectangle(bar23, BLUE);
	
	rectangle bar24 = {.left = 85, .right = 86, .top = 202-shift, .bottom = 204-shift};
	fill_rectangle(bar24, BLUE);
	rectangle bar25 = {.left = 85, .right = 86, .top = 204-shift, .bottom = 210-shift};
	fill_rectangle(bar25, WHITE);
	rectangle bar26 = {.left = 85, .right = 86, .top = 210-shift, .bottom = 213-shift};
	fill_rectangle(bar26, BLUE);
	
	rectangle bar27 = {.left = 86, .right = 87, .top = 203-shift, .bottom = 204-shift};
	fill_rectangle(bar27, BLUE);
	rectangle bar28 = {.left = 86, .right = 87, .top = 204-shift, .bottom = 206-shift};
	fill_rectangle(bar28, WHITE);
	rectangle bar29 = {.left = 86, .right = 87, .top = 206-shift, .bottom = 208-shift};
	fill_rectangle(bar29, GREY);
	rectangle bar30 = {.left = 86, .right = 87, .top = 208-shift, .bottom = 210-shift};
	fill_rectangle(bar30, WHITE);
	rectangle bar31 = {.left = 86, .right = 87, .top = 210-shift, .bottom = 214-shift};
	fill_rectangle(bar31, BLUE);
	
	rectangle bar32 = {.left = 87, .right = 88, .top = 204-shift, .bottom = 205-shift};
	fill_rectangle(bar32, BLUE);
	rectangle bar33 = {.left = 87, .right = 88, .top = 205-shift, .bottom = 207-shift};
	fill_rectangle(bar33, WHITE);
	rectangle bar34 = {.left = 87, .right = 88, .top = 206-shift, .bottom = 209-shift};
	fill_rectangle(bar34, GREY);
	rectangle bar35 = {.left = 87, .right = 88, .top = 208-shift, .bottom = 214-shift};
	fill_rectangle(bar35, BLUE);
	
	rectangle bar36 = {.left = 88, .right = 89, .top = 205-shift, .bottom = 213-shift};
	fill_rectangle(bar36, BLUE);
	
}

void clear_ghost_orange(uint8_t shift)
{
	rectangle bar200 = {.left = 95, .right = 109, .top = 201+shift, .bottom = 214+shift};
	fill_rectangle(bar200, YELLOW); //to delete
}

void init_ghost_orange(uint8_t shift)
{
	rectangle bar1 = {.left = 95, .right = 96, .top = 207+shift, .bottom = 213+shift};
	fill_rectangle(bar1, SHOCKINGPINK);
	rectangle bar2 = {.left = 96, .right = 97, .top = 204+shift, .bottom = 214+shift};
	fill_rectangle(bar2, SHOCKINGPINK);
	rectangle bar3 = {.left = 97, .right = 98, .top = 203+shift, .bottom = 214+shift};
	fill_rectangle(bar3, SHOCKINGPINK);
	
	rectangle bar4 = {.left = 98, .right = 99, .top = 202+shift, .bottom = 205+shift};
	fill_rectangle(bar4, SHOCKINGPINK);
	rectangle bar5 = {.left = 98, .right = 99, .top = 205+shift, .bottom = 209+shift};
	fill_rectangle(bar5, WHITE);
	rectangle bar6 = {.left = 98, .right = 99, .top = 209+shift, .bottom = 213+shift};
	fill_rectangle(bar6, SHOCKINGPINK);
	
	rectangle bar7 = {.left = 99, .right = 100, .top = 202+shift, .bottom = 204+shift};
	fill_rectangle(bar7, SHOCKINGPINK);
	rectangle bar8 = {.left = 99, .right = 100, .top = 204+shift, .bottom = 210+shift};
	fill_rectangle(bar8, WHITE);
	rectangle bar9 = {.left = 99, .right = 100, .top = 210+shift, .bottom = 212+shift};
	fill_rectangle(bar9, SHOCKINGPINK);
	
	rectangle bar10 = {.left = 100, .right = 101, .top = 203+shift, .bottom = 204+shift};
	fill_rectangle(bar10, SHOCKINGPINK);
	rectangle bar11 = {.left = 100, .right = 101, .top = 204+shift, .bottom = 206+shift};
	fill_rectangle(bar11, WHITE);
	rectangle bar12 = {.left = 100, .right = 101, .top = 206+shift, .bottom = 208+shift};
	fill_rectangle(bar12, GREY);
	rectangle bar13 = {.left = 100, .right = 101, .top = 208+shift, .bottom = 210+shift};
	fill_rectangle(bar13, WHITE);
	rectangle bar14 = {.left = 100, .right = 101, .top = 210+shift, .bottom = 213+shift};
	fill_rectangle(bar14, SHOCKINGPINK);
	
	rectangle bar15 = {.left = 101, .right = 102, .top = 201+shift, .bottom = 205+shift};
	fill_rectangle(bar15, SHOCKINGPINK);
	rectangle bar16 = {.left = 101, .right = 102, .top = 205+shift, .bottom = 206+shift};
	fill_rectangle(bar16, WHITE);
	rectangle bar17 = {.left = 101, .right = 102, .top = 206+shift, .bottom = 208+shift};
	fill_rectangle(bar17, GREY);
	rectangle bar18 = {.left = 101, .right = 102, .top = 208+shift, .bottom = 214+shift};
	fill_rectangle(bar18, SHOCKINGPINK);
	
	rectangle bar19 = {.left = 102, .right = 103, .top = 201+shift, .bottom = 214+shift};
	fill_rectangle(bar19, SHOCKINGPINK);
	
	rectangle bar20 = {.left = 103, .right = 104, .top = 201+shift, .bottom = 213+shift};
	fill_rectangle(bar20, SHOCKINGPINK);
	
	rectangle bar21 = {.left = 104, .right = 105, .top = 202+shift, .bottom = 205+shift};
	fill_rectangle(bar21, SHOCKINGPINK);
	rectangle bar22 = {.left = 104, .right = 105, .top = 205+shift, .bottom = 209+shift};
	fill_rectangle(bar22, WHITE);
	rectangle bar23 = {.left = 104, .right = 105, .top = 209+shift, .bottom = 212+shift};
	fill_rectangle(bar23, SHOCKINGPINK);
	
	rectangle bar24 = {.left = 105, .right = 106, .top = 202+shift, .bottom = 204+shift};
	fill_rectangle(bar24, SHOCKINGPINK);
	rectangle bar25 = {.left = 105, .right = 106, .top = 204+shift, .bottom = 210+shift};
	fill_rectangle(bar25, WHITE);
	rectangle bar26 = {.left = 105, .right = 106, .top = 210+shift, .bottom = 213+shift};
	fill_rectangle(bar26, SHOCKINGPINK);
	
	rectangle bar27 = {.left = 106, .right = 107, .top = 203+shift, .bottom = 204+shift};
	fill_rectangle(bar27, SHOCKINGPINK);
	rectangle bar28 = {.left = 106, .right = 107, .top = 204+shift, .bottom = 206+shift};
	fill_rectangle(bar28, WHITE);
	rectangle bar29 = {.left = 106, .right = 107, .top = 206+shift, .bottom = 208+shift};
	fill_rectangle(bar29, GREY);
	rectangle bar30 = {.left = 106, .right = 107, .top = 208+shift, .bottom = 210+shift};
	fill_rectangle(bar30, WHITE);
	rectangle bar31 = {.left = 106, .right = 107, .top = 210+shift, .bottom = 214+shift};
	fill_rectangle(bar31, SHOCKINGPINK);
	
	rectangle bar32 = {.left = 107, .right = 108, .top = 204+shift, .bottom = 205+shift};
	fill_rectangle(bar32, SHOCKINGPINK);
	rectangle bar33 = {.left = 107, .right = 108, .top = 205+shift, .bottom = 207+shift};
	fill_rectangle(bar33, WHITE);
	rectangle bar34 = {.left = 107, .right = 108, .top = 206+shift, .bottom = 209+shift};
	fill_rectangle(bar34, GREY);
	rectangle bar35 = {.left = 107, .right = 108, .top = 208+shift, .bottom = 214+shift};
	fill_rectangle(bar35, SHOCKINGPINK);
	
	rectangle bar36 = {.left = 108, .right = 109, .top = 205+shift, .bottom = 213+shift};
	fill_rectangle(bar36, SHOCKINGPINK);
	
}