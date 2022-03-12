#ifndef SOLAR_DL_H 
#define SOLAR_DL_H 
#include <stdint.h> 

// defines
#define CHECKSUM_INDEX 34 
#define CHECKSUM_NEW_INDEX 35 
#define DL_NUM_OF_BYTES 36

// structures 
struct dl_data 
{ 
// 0 Gerätekennung 90 hex 
// 1 Gerätekennung invertiert 6F hex 
// 2 don’t care für mögliche spätere Verwendung reserviert 
// 3 Zeitstempel: Minute akt. Zeitstempel der Regelung (Genauigkeit = 1Minute) 
// 4 Stunde 
// 5 Tag 
// 6 Monat 
// 7 Jahr Jahreszahl + 2000 (z.B.: 3 = 2003)
	uint8_t Kennung;
	uint8_t Kennung_inv;
	uint8_t xxx;
	uint8_t Min;
	uint8_t Std;
	uint8_t Tag;
	uint8_t Mon;
	uint8_t Jhr; 
// 8 Sensor1 low Temperatur oder Strahlung 
// 9 Sensor1 high höchstes Bit = Vorzeichen 
// 10 Sensor2 low Temperatur oder Strahlung 
// 11 Sensor2 high höchstes Bit = Vorzeichen 
// 12 Sensor3 low Temperatur oder Strahlung 
// 13 Sensor3 high höchstes Bit = Vorzeichen 
// 14 Sensor4 low Temperatur oder Strahlung 
// 15 Sensor4 high höchstes Bit = Vorzeichen 
// 16 Sensor5 low Temperatur oder Strahlung 
// 17 Sensor5 high höchstes Bit = Vorzeichen 
// 18 Sensor6 low Temperatur, Strahlung oder Volumenstrom (4l/h) 
// 19 Sensor6 high höchstes Bit = Vorzeichen
	int16_t Temp[6]; 
// 20 Ausgangsbyte 
// 21 Drehzahlstufe A1 Drehzahlstufe des Ausgang 1 
// 22 Analog_Ausgang Ausgangswert des Analogausgang (1/10V) 
// 23 Wärmemengenregister Bit0 gibt an, ob der Wärmemengenzähler aktiv ist
	uint8_t AusgByte;
	uint8_t DrehzA1;
	uint8_t AnaAusg;
	uint8_t WReg;
// 24 Volumenstrom 1l/h, Volumenstrom 
// 25 Volumenstrom 
// 26 Momentanleistung_low 1/10 kW, Wärmemengenzähler 
// 27 Momentanleistung_high 
// 28 KWh_low 1/10 kWh, Wärmemengenzähler 
// 29 KWh_high 
// 30 MWh_low_low 1 MWh, Wärmemengenzähler 
// 31 MWh_low_high 
// 32 MWh_high_low 
// 33 MWh_high_high
	uint16_t Lh;
	uint16_t KW;
	uint16_t KWh;
	uint16_t MWh_Lo;
        uint16_t MWh_Hi;
// 34 Prüfsumme SBytes mod 256 (= niederwertigsten 8 Bit der Summe) 
// 35 Prüfsumme (Neu)
	uint8_t PSum;
	uint8_t PSumNeu;
};

// prototypes 
void dl_init(void); 
int dl_calc( struct dl_data *dl);

#endif
