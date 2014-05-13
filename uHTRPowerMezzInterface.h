#ifndef UHTR_POWERMOD_INTERFACE
#define UHTR_POWERMOD_INTERFACE

#include "uHTRMezzInterface.h"

class uHTRPowerMezzInterface : public uHTRMezzInterface 
{
public:
    struct EEPROM_data 
    {
	uint8_t data_format_version; // this is version 1
	uint8_t mezz_type_code;      // see codes in the sections below
	uint8_t mezz_subtype_code;   // see codes in the sections below
	uint8_t mezz_type[16];       // String version of mezzanine type+sub type
	// (zero -terminated)
	uint8_t serial_number[2];    // construction serial number ([0]*256+[1])
	uint8_t manu_date[11];       // DAY-MON-YEAR (zero-terminated)
	uint8_t manu_site[16];       // Site of manufacture (zero-terminated)
	uint8_t manu_tester[16];     // Name of tester (zero-terminated)
	uint8_t test_release[8];     // Release version of test code used
	uint8_t notes[56];           // String notes field in this version,
    	                             // available for future use if needed
    };
    static const unsigned int EEPROM_DATA_SIZE = 128;
    
    uHTRPowerMezzInterface(int idev = 0, bool v2 = true, bool isRPi = true, const char host[] = "localhost", const char port[] = "1338");
    void configADC128();
    double readMezzADC(const int, const int);
    double readSUB20ADC(const int);
    void configMarginControl();
    void setMarginControl(const bool, const bool, const bool, const bool);
    //void releaseMarginControl();
    void readMarginPGood(bool*, bool*, bool*, bool* = 0);
    void togglePowerMezzs(const bool); 
    void toggleMezzsLoad(const int, const bool); 
    void disableMezz();
    void writeMezzEeprom(const EEPROM_data);
    void readMezzEeprom(EEPROM_data*);
    void printEEPROM_data(const EEPROM_data, const bool);
    void configGPIO();
};

#endif
