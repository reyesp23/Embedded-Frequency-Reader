/*
 * Funciones_FrequencyReader.h
 *
 * Created: 21/10/2018 07:05:38 p. m.
 *  Author: Patricio
 */ 

#ifndef FUNCIONES_FREQUENCY_READER_H_
#define FUNCIONES_FREQUENCY_READER_H_
#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>

#define LEDs PORTB
#define BTNs_PORT PORTA
#define BTNs_PIN PINA
#define BTN_mas 0
#define BTN_menos 1
#define BTN_on_off 7

#endif
