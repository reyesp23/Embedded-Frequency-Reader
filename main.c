/*
 * FrequencyReader.c
 *
 * Created: 18/10/2018 10:50:49 a. m.
 * Author : Patricio Reyes
 */

#include "Subrutinas/Funciones_FrequencyReader.h"

unsigned char ON_FLAG = 1; //System is ON/OFF
unsigned char READ_BUTTONS_FLAG = 0; //Read buttons (enabled every 20ms)
unsigned char BUTTONS_01_ENABLED_FLAG = 1; //Enable buttons 0 and 1 (when the system is ON)
unsigned char BTN_7_ACTIVE_FLAG = 0; //Button 7 is pressed
unsigned char BTN_0_ACTIVE_FLAG = 0; //Button 0 is pressed
unsigned char BTN_1_ACTIVE_FLAG = 0; //Button 1 is pressed
unsigned char INT0_EXT_FLAG = 0; //External interrupt INT0 is active
unsigned char T0_COUNTER_50 = 0; //Counter reached 50 = 500ms
unsigned char T0_COUNTER_02 = 0; //Counter reached 2 = 20ms
unsigned char LED_7_STATUS = 0; //LED7 is ON/OFF
unsigned char NEGADO_FLAG = 0; //The output should be inverted/non-inverted
unsigned char FREQ_LEVEL = 0; //Frequency level (PWM Output)
unsigned char FREQ_INDICATOR; //Frequency level read in the ICU
unsigned int IC_TIMEOUT = 0; //Verify if the ICU is receiving a signal
unsigned char FIRST_MEASUREMENT_FLAG = 0; //First ICU measurement completed
unsigned char SECOND_MEASUREMENT_FLAG = 0; //Second ICU measurement completed
unsigned int t; //Save the period of the signal read on the ICU

int main(void) {
    DDRA = 0x00; //PORT A Input (Buttons)
    DDRB = 0x87; //PORT B Output (LEDs 7 and 0-2)
    DDRD = 0x80; //D7 Output (PWM), D6 Input (ICU), D2 Input (INT0)
    
    BTNs_PORT = 0xFF; //Port A, pull-up resistors
    LEDs = 0xFF; //Port B, start off
    PORTD = 0x7F; //Port D, pull-up resistor on D7
    
    //Timer 2 Fast PWM
    TCCR2 = 0x6F; //Fast PWM, non inverted
    OCR2=127; //Duty Cycle 50%
    
    //Timer0 Interrupt (Count time: 10ms)
    TCCR0 = 0x05; //Normal mode, pre-scaler 1024
    TCNT0 = 0xB2; //Count for 10ms
    //Timer1 Interrupt (Input Capture Unit)
    TCCR1A = 0; //Normal Mode
    TCCR1B = 0x05; //Falling edge, pre-scaler 1024, no NC
    TIMSK = (1<<TOIE0) | (1<<TICIE1); //Enable Interrupts: OVF de T0 y ICU de T1
    //External Interrupt INT0
    MCUCR = (1<<ISC01); //Falling edge
    GICR = (1<<INT0); //Enable external interrupt INT0
    sei(); //Enable global interrupts
    
    while (1) {
        /*---------------------SECTION 1: TURNED ON ---------------------*/
        /*The code in this section runs only when the ON_FLAG is active.
         Includes: PWM, Input Capture Unit, enable buttons 0 and 1, output on LEDs. */
        
        if (ON_FLAG) { //The system is ON
            BUTTONS_01_ENABLED_FLAG = 1; //Enable buttons 0 and 1
            TCCR1B = 0x05; //Turn on Timer 1:Input Capture Unit
            /*---------------------BLINKING LED 7---------------------*/
            if (T0_COUNTER_50 == 50) { //If the counter reaches 50 (50*10ms) = 500ms
                T0_COUNTER_50 = 0; //Restart counter (50)
                if (LED_7_STATUS) { //If the LED 7 Status is ON
                    LED_7_STATUS = 0; //Invert LED STATUS
                } else { //If the LED STATUS is off
                    LED_7_STATUS = 1; //Invert LED STATUS
                }
            }
            
            if (LED_7_STATUS) { // If the STATUS is 1, the LED is on
                FREQ_INDICATOR &=0x7F; //Turn off LED 7
            } else { //If the STATUS is 0, the LED is off
                FREQ_INDICATOR|=0x80; //Turn On LED 7
            }
            
            /*---------------------PRINT MEASUREMENT ON LEDs---------------------*/
            if (NEGADO_FLAG) { //If the inverted flag is On
                LEDs = FREQ_INDICATOR;	//Print the result with inverted output
            } else { //If the inverted flag is Off
                LEDs = ~FREQ_INDICATOR;	//Print the result with normal output
            }
            
            /*---------------------GENERATE PWM---------------------*/
            switch(FREQ_LEVEL) { //Change Timer2 pre-scalers
                case(0):
                    TCCR2 = 0; //Turn off Timer2, no PWM
                    break;
                case(1):
                    TCCR2 = 0x6F; //Pre-scaler 1024:30 Hz
                    break;
                case(2):
                    TCCR2 = 0x6E; //Pre-scaler 256: 120 Hz
                    break;
                case(3):
                    TCCR2 = 0x6C;  //Pre-scaler 64: 482 Hz
                    break;
                case(4):
                    TCCR2 = 0x6A; //Pre-scaler 8: 3.8 KHz
                    break;
                case(5):
                    TCCR2 = 0x69; //No pre-scaler: 30.9 KHz
                    break;
            }
            
            /*---------------------INPUT CAPTURE UNIT---------------------*/
            if (SECOND_MEASUREMENT_FLAG) { //If second measurement is complete
                SECOND_MEASUREMENT_FLAG=0; //Reset the second measurement flag
                //Use the period to determine the PWM signal
                if (t<1) {
                    FREQ_INDICATOR = 5; //Print 5 on LED
                }
                if ((3>=t)&&(t>=2)) {
                    FREQ_INDICATOR = 4; //Print 4 on LED
                }
                if ((17>=t)&&(t>=15)) {
                    FREQ_INDICATOR = 3; //Print 3 on LED
                }
                if ((65>=t)&&(t>=64)) {
                    FREQ_INDICATOR = 2; //Print 2 on LED
                }
                if ((256>=t)&&(t>=100)) {
                    FREQ_INDICATOR = 1; //Print 1 on LED
                }
                if (t>256) {
                    FREQ_INDICATOR = 0;
                }
            }
            /*---------------------TIMEOUT INPUT CAPTURE UNIT---------------------*/
            IC_TIMEOUT++; //Increase by 1 the timeout on each cycle
            if (IC_TIMEOUT > 10000) { //If the timeout is larger than 10,000 the ICU is not reading
                FREQ_INDICATOR = 0; //Measured frequency is 0
            }
            /*---------------------SECTION 2: TURNED OFF---------------------*/
            /*Code in this section runs when the system is turned off.
             Includes: Turn off LEDs, disable buttons 0 and 1, disable Timers 1 (ICU) and 2 (PWM)*/
        } else { //System off
            LEDs = 0xFF; //Turn off LEDs
            TCCR2 = 0; //Turn off Timer2 PWM
            TCCR1B =0; //Turn off Timer 1 ICU
            BUTTONS_01_ENABLED_FLAG = 0; //Disable buttons 1 y 0
            LED_7_STATUS = 1; //Reset flag LED7
            T0_COUNTER_50 = 50; //Reset 500ms counter
        }
        /*---------------------SECTION 3: ALWAYS RUNNING---------------------*/
        /*The code in this section is running always.
         Include: Read buttons, external interrupt */
        
        /*--------------------- READ BUTTONS (POLLING EVERY 20ms) ---------------------*/
        /*20ms Counter*/
        if (T0_COUNTER_02 == 2) { //If the counter reaches 2 (2*10ms) = 20ms
            T0_COUNTER_02 = 0; //Reset counter (2)
            READ_BUTTONS_FLAG = 1; //Activate flag to read buttons
        }  /*END: 20ms counter*/
        /*READ BUTTONS*/
        if (READ_BUTTONS_FLAG) { //If the read buttons flag is on
            //PUSH 7
            if ((BTNs_PIN|~(1<<BTN_on_off))==~(1<<BTN_on_off)) { //The button was pressed
                BTN_7_ACTIVE_FLAG = 1; //Set the button as active
            }
            if (BTN_7_ACTIVE_FLAG == 1) { //If the button is active
                if (((BTNs_PIN|~(1<<BTN_on_off))==~(1<<BTN_on_off))==0) { //Release the button
                    BTN_7_ACTIVE_FLAG = 0; //Set the button as inactive
                    if (ON_FLAG) { //If the system is ON
                        ON_FLAG = 0; //Turn off the system
                    } else { //If the system is OFF
                        ON_FLAG = 1; //Turn on the system
                    }
                }
            } /*END: PUSH 7*/
            
            /*BUTTONS 0 Y 1 ENABLED*/
            if (BUTTONS_01_ENABLED_FLAG) { //If buttons 0 and 1 are enabled
                //PUSH 0
                if ((BTNs_PIN|~(1<<BTN_mas))==~(1<<BTN_mas)) { //The button was pressed
                    BTN_0_ACTIVE_FLAG = 1; //Set the button as active
                }
                if (BTN_0_ACTIVE_FLAG == 1) { //If the button is active
                    if (((BTNs_PIN|~(1<<BTN_mas))==~(1<<BTN_mas))==0) { //Release the button
                        BTN_0_ACTIVE_FLAG = 0; //Set the button as inactive
                        if (FREQ_LEVEL<5) { //If the level is less than 5
                            FREQ_LEVEL++; //Increase the level
                        }
                    }
                } /*END: PUSH 0*/
                //PUSH 1
                if ((BTNs_PIN|~(1<<BTN_menos))==~(1<<BTN_menos)) { //The button was pressed
                    BTN_1_ACTIVE_FLAG = 1; //Set the button as active
                }
                if (BTN_1_ACTIVE_FLAG == 1) { //If the button is active
                    if (((BTNs_PIN|~(1<<BTN_menos))==~(1<<BTN_menos))==0) { //Release the button
                        BTN_1_ACTIVE_FLAG = 0; //Set the button as inactive
                        if (FREQ_LEVEL>0) { //If the level is greater than 0
                            FREQ_LEVEL--; //Decrease the level
                        }
                    }
                } /*END: PUSH 1*/
            } /*END: Buttons 0 and 1 ENABLED*/
            READ_BUTTONS_FLAG = 0; //Disable read buttons
        } /*END: LECTURA DE BOTONES*/
        
        /*---------------------EXTERNAL INTERRUPT INT0---------------------*/
        if (INT0_EXT_FLAG) { /*INT0 ACTIVE*/
            if (NEGADO_FLAG) { //If the inverted flag is active
                NEGADO_FLAG = 0; //Disable inverted flag
            } else {
                NEGADO_FLAG = 1; //Enable inverted flag
            }
            INT0_EXT_FLAG = 0; //Disable the external interrupt flag
        }/*END: INT0 ACTIVA*/
    } /*END: while(1)*/
}/*END: main*/

/*---------------------SECTION 4: INTERRUPT SUBRUTINES---------------------*/

ISR(TIMER0_OVF_vect) { /*OVERFLOW TIMER 0*/
    TCNT0 = 0xB2; //Load the Timer value
    T0_COUNTER_50++; //Increase the blinking counter (50)
    T0_COUNTER_02++; //Increase the button counter (2)
}

ISR(TIMER1_CAPT_vect) { /*INPUT CAPTURE TIMER 1*/
    IC_TIMEOUT = 0; //Reset input capture timeout
    if (FIRST_MEASUREMENT_FLAG == 0) { //First measurement not yet taken
        t=ICR1; //Save the first measurement
        FIRST_MEASUREMENT_FLAG = 1; //First measurement complete
    }
    else { //First measurement already taken
        t = ICR1-t; //Take the first measurement and substract it from the second measurement
        FIRST_MEASUREMENT_FLAG = 0; //First measurement = 0
        SECOND_MEASUREMENT_FLAG = 1; //Second measurement complete
    }
}
ISR(INT0_vect) { /*EXTERNAL INTERRUPT INT0 (PIN D2)*/
    INT0_EXT_FLAG = 1; //External interrupt flag is on
}
