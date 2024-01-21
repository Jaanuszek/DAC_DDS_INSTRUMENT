#include "MKL05Z4.h"
#include "i2c.h"
#include "lcd1602.h"
#include "klaw.h"
#include "DAC.h"
#include "tsi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#define DIV_CORE 2048 //fclk = 2048Hz, df = 2Hz
#define MASKA_10BIT 0x03FF
#define ZYXDR_Mask 1<<3 // Maska bitu ZYXDR, bit ten informuje czy dane sa gotowe do odczytu

//zmienne do przycisków
volatile uint8_t S2_press=0;
volatile uint8_t S3_press=0;

//zmmienne do konwentera C/A
int16_t temp;
int16_t Sinus[1024];
uint16_t slider;
uint16_t dac;
uint16_t faza, mod, freq, df;
uint16_t oktawa[] ={32,16,8,4,2,1};
uint16_t dzwiek[] ={262,294,330,350,392,440,494};

uint8_t ton=0;
int8_t gamma=5;
int8_t i;

char *nazwyOktaw[]={"Sub kontra", "Kontra", "Duza", "Mala", "Razkreslna", "Dwukreslna"};
char *nazwyDzwiekow[]={"C","D","E","F","G","A","H"};

//zmienne do akcelerometru
static uint8_t arrayXYZ[6];
static uint8_t sens;
static uint8_t status;
double xAcceleration;
volatile uint8_t delay=0; //zmienna sprawdzajaca czy systick ma dzialac jako opoznienie czy jako generator sinusoidy
uint8_t ms_ok=0;
uint8_t czas=0;

//obsluga przerwania od SysTick
void SysTick_Handler(void){
	//if(!delay){ // jezeli systick ma generowac sygnal dac
		dac=(Sinus[faza]/100)*slider+0x0800; 
		DAC_Load_Trig(dac);
		faza+=mod; //generator cyfrowej fazy
		faza&=MASKA_10BIT; //rejestr liczacy  2^N bitów (N=10 w tym przypadku)
	//} // jezeli nie, to sluzy jako zwykle opoznienie
	//else{
	//	ms_ok=1;
	//}
}

//obsluga przerwania od GPIO, w tym przypadku od przycisków klawiatury
void PORTA_IRQHandler(void){
	uint32_t buf;
	buf=PORTA->ISFR & (S2_MASK | S3_MASK); // wpisanie do zmiennej buf stany pinow portu A
	
	switch(buf)
	{
		case S2_MASK:
			DELAY(10); //minimalizacja drgan zestytkow
			if(!(PTA->PDIR&S2_MASK)){//minimalizacja drgan zestytkow
				if(!(PTA->PDIR&S2_MASK))//minimalizacja drgan zestytkow
				{
					if(!S2_press){
						S2_press=1;
					}
				}
			}
			break;
		case S3_MASK:
			DELAY(10);
			if(!(PTA->PDIR&S3_MASK)){
				if(!(PTA->PDIR&S3_MASK)){
					if(!S3_press){
						S3_press=1;
					}
				}
			}
			break;
		default: break;
	}
	PORTA->ISFR |= S2_MASK|S3_MASK; //Zerowanie bitów ISF, w celu eliminacji falszywych przerwan
	NVIC_ClearPendingIRQ(PORTA_IRQn);
}
void showOctave(char display[]){ //wyswietla na ekranie nazwy oktawy
	sprintf(display,"%s", nazwyOktaw[gamma]);
	LCD1602_SetCursor(0,0);
	LCD1602_ClearAll();
	LCD1602_Print(display);
}
void calculateFrequency(char display[]){ // funkcja odpowiadajaca za obliczenie czestotliwosci i wyswietlenie jej na ekranie
	mod=dzwiek[i]/oktawa[gamma]; // obliczenie modulatora fazy
	freq=mod*df; //obliczenie czestotliwosci
	LCD1602_SetCursor(0,1);
	sprintf(display,"Freq=%04dHz",freq);
	LCD1602_Print(display);
	sprintf(display,"%s",nazwyDzwiekow[i]);
	LCD1602_SetCursor(12,1);
	LCD1602_Print(display);
}
void SysTickDelay(uint8_t prescaler){ //funkcja odpowiadajaca za opóznienie wywolane przerwaniem systicka
	SysTick_Config(SystemCoreClock/prescaler); 
			while(czas<1){
				if(ms_ok){
					delay = 0;
					SysTick_Config(SystemCoreClock/DIV_CORE);
					ms_ok=0;
					czas=1;
				}
			}
			czas=0;
}

int main(void){
	uint8_t w;
	slider=50;
	df=DIV_CORE/MASKA_10BIT; // Rozdzielczosc generatora
	char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
	
	klaw_INIT(); // Inicjalizacja klawiatury
	klaw_INT(); // Wlaczenie przerwan od klawiatury
	TSI_Init(); // Inizjalizacja panelu dotykowego
	LCD1602_Init(); // Inicjalizacja wyswietlacza LCD
	LCD1602_Backlight(TRUE);
	
	DAC_Init(); // Inicjalizacja przetrwornika C/A
	for(faza=0; faza<1024;faza++){
		Sinus[faza]=(sin((double)faza*0.0061359231515)*2047.0); // Wpisanie do tablicy kolejnych 1024 probek funkcji sinus
	}
	faza=0;
	mod=dzwiek[i]/oktawa[gamma];
	freq=mod*df;
	sprintf(display,"%s", nazwyOktaw[gamma]);
	LCD1602_SetCursor(0,0);
	LCD1602_Print(display);  // Wyswietlenie nazwy okatawy
	LCD1602_SetCursor(0,1);
	sprintf(display,"Freq=%04dHz",freq); // Wyswietlenie czestotliwosci danego dzwieku
	LCD1602_Print(display);
	calculateFrequency(display);
	SysTick_Config(SystemCoreClock/DIV_CORE); // Wlaczenie przerwan od SysTicka

	sens=0;	// Wybor czulosci
	I2C_WriteReg(0x1d, 0x2a, 0x0); // Rejestr konfiguracyjny CTRL_REG1 ustawiony na stan czuwania
	I2C_WriteReg(0x1d, 0xe, sens); // Rejestr konfiguracyjny XYZ_DATA_CFG ustawiony na wybrana czulosc
	I2C_WriteReg(0x1d, 0x2a, 0x1); // Rejestr konfiguracyjny CTRL_REG1 ustawiony na stan aktywny	
	
 while(1) {
	 w=TSI_ReadSlider(); // Zczytanie pozycji panelu dotykowego
	 if(w!=0){
		 slider=w;
	 }
	 if(S2_press){ // Zwiekszanie oktawy
		temp=mod;
		i=0;
		gamma+=1;
		if(gamma==6){
			gamma=5;
		}
		showOctave(display);
		calculateFrequency(display);
		S2_press=0;
	 }
	 if(S3_press){ //Zmniejszanie oktawy
			temp=mod;
			i=0;
			gamma-=1;
			if(gamma==(-1)){
				gamma=0;
			}
			showOctave(display);
			calculateFrequency(display);
			S3_press=0;
		}
	  I2C_ReadReg(0x1d, 0x0, &status); // Odczytanie rejestru stanu STATUS
		status&=ZYXDR_Mask; // Sprawdzenie czy dane sa gotowe do odczytu (sprawdzenie bitu ZYXDR)
	 if(status){ // Jezeli dane sa gotowe do odczytu to wykonuje sie ta instrukcja
		 I2C_ReadRegBlock(0x1d, 0x1, 6, arrayXYZ); // Odczyt blokowy 6 rejestrow
		 // Stworzenie jednej 14 bitowej wartosci z dwoch 8-bitowych
		 xAcceleration= ((double)((int16_t)((arrayXYZ[0]<<8)|arrayXYZ[1])>>2)/(4092>>sens));
		 if(xAcceleration >1&&xAcceleration<2){ // Jezeli przyspieszenie miesci sie w tym zakresie to zwieksza wysokosc dzwieku
			 delay = 1;
				i+=1;
				if(i>6){
					i=6;
				}
				calculateFrequency(display);
				//SysTickDelay(20);
		 }
		 else if (xAcceleration<(-1) && xAcceleration>(-2)){ //zmniejszenie wysokosci dzwieku
				delay = 1;
				i-=1;
				if(i<0){
					i=0;
				}
				calculateFrequency(display);
				//SysTickDelay(20);
		 }
	 }
 }
}