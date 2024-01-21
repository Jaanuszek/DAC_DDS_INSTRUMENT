#include "DAC.h"

void DAC_Init(void)
{
	SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK;          // Dołączenie sygnału zegara do DAC0
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
	DAC0->C0 |= (DAC_C0_DACEN_MASK | DAC_C0_DACTRGSEL_MASK);
}

uint8_t DAC_Load_Trig(uint16_t load)
{
	uint8_t load_temp;
	if(load>0xFFF)	return (1);		// Sprawdzenie zakresu danej wejściowej
	load_temp=load&0x0FF;
	DAC0->DAT->DATL = load_temp;	// Załadowanie młodszego bajtu przetwornika C/A
	load_temp=(load>>8);
	DAC0->DAT->DATH = load_temp;	// Załadowanie starszego bajtu przetwornika C/A
	DAC0->C0 |= DAC_C0_DACSWTRG_MASK;		// Wyzwolenie przetwarzania przetwornika C/A
	return (0);
}
