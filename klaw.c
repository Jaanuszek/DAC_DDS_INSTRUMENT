#include "klaw.h"

void klaw_INIT(void){ //wlaczenie pinow
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK; //wlaczenie portu A
	PORTA->PCR[S2]	|= PORT_PCR_MUX(1); //w pliku klaw.h stworzono definicje pinow zeby bylo wygodniej
	PORTA->PCR[S3]	|= PORT_PCR_MUX(1); //wlaczenie rezystora podciagajacego i przypiecie go do vdd
	PORTA->PCR[S2]	|= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
	PORTA->PCR[S3]	|= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
}

void klaw_INT(void){ //wywolanie przerwania
	PORTA -> PCR[S2] |= PORT_PCR_IRQC(0xa);
	PORTA -> PCR[S3] |= PORT_PCR_IRQC(0xa);
	NVIC_ClearPendingIRQ(PORTA_IRQn); //wyczyszczenie potencjalnego przerwania "just in case"
	NVIC_EnableIRQ(PORTA_IRQn); //wlaczenie danego przerwania, w tym wypadku przerwanie od gpio na port a
}