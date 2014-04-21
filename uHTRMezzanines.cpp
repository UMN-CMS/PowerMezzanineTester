#include "uHTRMezzanines.h"

std::map<std::string, std::pair<unsigned int, unsigned int> > Mezzanine::snList;
unsigned int Mezzanine::sn_max[4];

Mezzanine::Mezzanine(uHTRPowerMezzInterface& s, const int muxAddress, const double vout, const bool ispm, bool V2, int aMuxAdd) : s20(s), muxAddr(muxAddress), aMuxAddr(aMuxAdd), isV2(V2), isPM(ispm), VOUT_NOM(vout)
{
    actTest = &margNom;
    isMaybeNotThere = false;
    isNotThere = false;
    isInit = false;
    margin = 0;
    load = 0;
    f_detail = NULL;
    margUp.marg_state =  MARGIN_OFFSET;
    margDn.marg_state = -MARGIN_OFFSET;

    if(Mezzanine::snList.size() == 0) loadSSNFile(0);
}

Mezzanine::~Mezzanine()
{
    setRun(false);
    if(f_detail) fclose(f_detail);
}

void Mezzanine::setMargins(const int marg, const int l)
{
    margin = marg;
    load = l;
    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);

    // configure margin control GPIO
    s20.configMarginControl();

    if(marg < 0)
    {
        // Margin down test 

        // PCA9534; SLAVE ADDRESS: 0100 000
        // Channel 1: MARG_UP = 0
        // Channel 2: MARG_DWN = 1
        s20.setMarginControl(false, true, false, false);

        // set summary to write to
        actTest = &margDn;
    }
    else if(marg > 0)
    {
        // Margin up test

        // PCA9534; SLAVE ADDRESS: 0100 000
        // Channel 1: MARG_UP = 1
        // Channel 2: MARG_DWN = 0
        s20.setMarginControl(false, false, true, false);

        // set summary to write to
        actTest = &margUp;
    }
    else
    {
        // Nominal margin test

        // PCA9534; SLAVE ADDRESS: 0100 000
        // Channel 1: MARG_UP = 0
        // Channel 2: MARG_DWN = 0
        s20.setMarginControl(false, false, false, false);

        // set summary to write to
        if(load == 2) actTest = &hlNom;
        else          actTest = &margNom;
    }
}

void Mezzanine::setRun(const bool run)
{
    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);

    s20.togglePowerMezzs(run);
}

void Mezzanine::readEeprom()
{
    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);

    //activate the power mezzanine so the eeprom is powered
    setRun(true);
    usleep(100000); // Sleep to ensure mezzanine is fully powered

    char slot[16] = "";
    switch(muxAddr)
    {
    case V2_MUX_PM_3_3:
	sprintf(slot, "PM 3.3V");
	break;
    case V2_MUX_PM_1_A:
	sprintf(slot, "PM 1.0V A");
	break;
    case V2_MUX_PM_1_B:
	sprintf(slot, "PM 1.0V B");
	break;
    case V2_MUX_APM_2_5:
	sprintf(slot, "APM 2.5V");
	break;
    case V2_MUX_APM_1_6:
	sprintf(slot, "APM 1.8V");
	break;
    }

    if(!s20.readMezzMAC())
    {
        if(isPM) printf("Power Mezzanine %s EEPROM info readback\n", slot);
        else printf("Aux Power Mezzanine %s EEPROM info readback\n", slot);
        s20.printMezzMAC();
        uHTRPowerMezzInterface::EEPROM_data t_data;
        s20.readMezzEeprom(&t_data);
        s20.printEEPROM_data(t_data, false);
        printf("\n");
    }
    else
    {
        if(isPM) printf("No Power Mezzanine %s Detected.\n", slot);
        else printf("No Aux Power Mezzanine %s Detected.\n", slot);
	if(s20.isRPi_)
	{
	    s20.printMezzMAC();
	}
	printf("\n");
    }

    //deactivate power mezzanines
    setRun(false);
}

void Mezzanine::loadSSNFile(const unsigned int skipBlockSize)
{
    FILE * f_SN = NULL;
    for(int i = 0; i < 4; i++) Mezzanine::sn_max[i] = 0;
    if((f_SN = fopen("serialNumberRecord.txt", "r")))
    {
        char buff[4096];
        char *c;
        while(!feof(f_SN) && (c = fgets(buff, 4096, f_SN)) != NULL)
        {
            char mac[18];
            unsigned int sn, mezzType;
            for(char* k = strchr(buff, ','); k != 0; k = strchr(buff, ',')) *k = ' ';
            if(buff[0] != '#' && (sscanf(buff, "%d %d %s\n", &mezzType, &sn, mac) == 3))
            {
                if(mezzType < 4)
                {
                    Mezzanine::snList[std::string(mac)] = std::make_pair(mezzType, sn);
                    if(sn > Mezzanine::sn_max[mezzType]) Mezzanine::sn_max[mezzType] = sn;
                }
                else printf("Invalid mezzanine type in serial number file for mezzanine %s.\n", mac);
            }
        }
        fclose(f_SN);

        // If a block of SNs is specified to be skipped increment sn_max so skipBlockSize SNs are skipped
        //*sn_max += skipBlockSize;
    }
    else
    {
        printf("!!!Serial number file not found, dummy SNs will be written (0xffff)!!!\n");
        for(int i = 0; i < 4; i++) Mezzanine::sn_max[i] = (unsigned int)(-1);
    }
}

unsigned int Mezzanine::getSN(const char * const cmac, const mezzLabel ml)
{
    if(Mezzanine::sn_max[ml] == (unsigned int) - 1)
    {
        printf("!!!Invalid serial number warning, no entry into database will be made!!!\n");
        return 0xffff;
    }

    unsigned int sn = 0xffff;
    char hmac[20];
    sprintf(hmac, "%02x:%02x:%02x:%02x:%02x:%02x", 0xff & cmac[0], 0xff & cmac[1], 0xff & cmac[2], 0xff & cmac[3], 0xff & cmac[4], 0xff & cmac[5]);
    std::string mac(hmac);
    if(Mezzanine::snList.find(mac) == Mezzanine::snList.end())
    {
        // This is a new SN and therefore must be added to the table
        FILE * f_SN = NULL;
        if((f_SN = fopen("serialNumberRecord.txt", "a")))
        {
            sn = ++(Mezzanine::sn_max[ml]);
            Mezzanine::snList[mac] = std::make_pair(ml, sn);
            fprintf(f_SN, "%d %d %s\n", ml, 0xffff & sn, mac.c_str());
            fclose(f_SN);
        }
        else
        {
            printf("!!!Serial number file write error, dummy SNs will be written (0xffff)!!!\n");
            return 0xffff;
        }
    }
    else sn = Mezzanine::snList[mac].second;

    return sn;
}

void Mezzanine::Summary::translateStatus(const unsigned int status, char *error)
{
    if(status == RETVAL_SUCCESS)
    {
        sprintf(error, "OK");
        return;
    }
    else if(status & RETVAL_ABORT)              sprintf(error, "ABORT");
    else if(status & RETVAL_BAD_SUPPLY_VOLTAGE) sprintf(error, "12V BAD");
    else if(status & RETVAL_BAD_3_3VOLT)        sprintf(error, "3.3V BAD");
    else if(status & RETVAL_BAD_POWERBALANCE)   sprintf(error, "MISBAL");
    else if(status & RETVAL_BAD_VOUT)           sprintf(error, "BAD VOUT");
    else if(status & RETVAL_BAD_VA)             sprintf(error, "BAD VA");
    else if(status & RETVAL_BAD_VB)             sprintf(error, "BAD VB");
    else if(status & RETVAL_BAD_VC)             sprintf(error, "BAD VC");
    else if(status & RETVAL_BAD_IA)             sprintf(error, "BAD IA");
    else if(status & RETVAL_BAD_IB)             sprintf(error, "BAD IB");
    else if(status & RETVAL_BAD_IC)             sprintf(error, "BAD IC");
    else if(status & RETVAL_BAD_VADJ)           sprintf(error, "BAD VADJ");
    else if(status & RETVAL_BAD_POWER)          sprintf(error, "BADPOWER");
    else if(status & RETVAL_NO_MODLE_DETECTED)  sprintf(error, "NO MOD");
}

unsigned int PM::monitor(bool passive)
{
    double p_temp = 0.0, p_vout = 0.0;
    double I_A = 0.0, V_A = 0.0, I_B = 0.0, V_B = 0.0;
    bool p_margup, p_margdn, p_pgood;

    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);

    if(!passive) s20.updateSUB20Display("\fFinished setMUXChannel");
    //--------------------------------------------------------------------
    // Print POWER MODULE MAC address
    //--------------------------------------------------------------------
    // 24AA02E48T; SLAVE ADDRESS: 1010 000
    if(!s20.readMezzMAC())
    {
        if(!passive) s20.updateSUB20Display("\fMAC ADDRESS OK");
        s20.copyMezzMAC(actTest->MAC);

        // Open pm file if not open already
        if(f_detail == NULL && !passive)
        {
            char fname[128];
            sprintf(fname, "PowerMezz-%02x-%02x-%02x-%02x-%02x-%02x.txt", actTest->MAC[0]&0xff, actTest->MAC[1]&0xff, actTest->MAC[2]&0xff, actTest->MAC[3]&0xff, actTest->MAC[4]&0xff, actTest->MAC[5]&0xff);
            f_detail = fopen(fname, "w");
            fprintf(f_detail, "%25s, %17s, %7s, %9s, %5s, %7s, %7s, %5s, %9s, %9s, %9s, %9s, %9s\n", "Date", "MAC", "temp", "Vout", "PGOOD", "MARG_UP", "MARG_DN", "Load", "I_A", "V_A", "I_B", "V_B", "Total Power");
        }

        if(!passive)
        {
            // Print Timestamp
            time_t epch = time(0);
            char ts[128];
            int i = 0;
            sprintf(ts, "%s", asctime(gmtime(&epch)));
            while(ts[i] != '\n') i++;
            ts[i] = '\0';

            fprintf(f_detail, "%25s, ", ts);
            // Print MAC
            fprintf(f_detail, "%02x:%02x:%02x:%02x:%02x:%02x, ", actTest->MAC[0]&0xff, actTest->MAC[1]&0xff, actTest->MAC[2]&0xff, actTest->MAC[3]&0xff, actTest->MAC[4]&0xff, actTest->MAC[5]&0xff);
        }

        //--------------------------------------------------------------------
        // Print POWER MODULE temperature
        //--------------------------------------------------------------------
        // MCP3426A0; SLAVE ADDRESS: 1101 000
        // Channel 1: TEMP(degrees C) = 25 + (V_CH1 - 750) / 10
        // 2.048V reference -> 1 mV per LSB
        if(isV2) p_temp = 25 + (s20.readMezzADC(V2_MEZZ_ADC, 1) - 750.0) / 10.0;
        else     p_temp = 25 + (s20.readMezzADC(ADC_26, 1) - 750.0) / 10.0;
        if(!passive) fprintf(f_detail, "%5.1f C, ", p_temp);

        //--------------------------------------------------------------------
        // Print POWER MODULE V_OUT voltage
        //--------------------------------------------------------------------
        // MCP3426A0; SLAVE ADDRESS: 1101 000
        // Channel 2: V_OUT = 4.322 * V_CH2
        // 2.048V reference -> 1 mV per LSB
        if(isV2) p_vout = 4.322 * s20.readMezzADC(V2_MEZZ_ADC, 2) / 1000;
        else     p_vout = 4.322 * s20.readMezzADC(ADC_26, 2) / 1000;
        if(!passive) fprintf(f_detail, "%7.3f V, ", p_vout);

        //--------------------------------------------------------------------
        // Print POWER MODULE PGOOD status
        // Print POWER MODULE MARG_UP status
        // Print POWER MODULE MARG_DWN status
        //--------------------------------------------------------------------
        // PCA9534; SLAVE ADDRESS: 0100 000
        // Channel 7: PGOOD
        // Channel 2: MARG_UP
        // Channel 3: MARG_DWN
        s20.readMarginPGood(&p_margup, &p_margdn, &p_pgood);
        if(!passive) fprintf(f_detail, "%5d, %7d, %7d, %5d, ", p_pgood, p_margup, p_margdn, load);

        //--------------------------------------------------------------------
        // Print POWER MODULE I_A voltage
        //--------------------------------------------------------------------
        if(isV2)
        {
            // MCP3426A6; SLAVE ADDRESS: 1101 110
            // Channel 1: I_A = 2.5 * V_CH0
            I_A = 2.5 * s20.readMezzADC(V2_PM_ADC, 1) / 1000;
        }
        else
        {
            // MCP3428; SLAVE ADDRESS: 1101 010
            // Channel 1: I_A = 2 * V_CH1
            // 2.048V reference -> 1 mV per LSB
            I_A = 2 * s20.readMezzADC(ADC_28, 1) / 1000;
        }
        if(!passive) fprintf(f_detail, "%7.3f A, ", I_A);

        //--------------------------------------------------------------------
        // Print POWER MODULE V_A voltage
        //--------------------------------------------------------------------
        if(isV2)
        {
            // 12 V input ADC_5 on sub20 (V_A = 9.2 * V_ADC5)
            if(s20.isRPi_) V_A = 9.2 * s20.readSUB20ADC(7 - 2*(aMuxAddr-1));
	    else           V_A = 9.2 * s20.readSUB20ADC(6);
        }
        else
        {
            // MCP3428; SLAVE ADDRESS: 1101 010
            // Channel 2: V_A = 18.647 * V_CH2
            // 2.048V reference -> 1 mV per LSB
            V_A = 18.647 * s20.readMezzADC(ADC_28, 2) / 1000;
        }
        if(!passive) fprintf(f_detail, "%7.3f V, ", V_A);

        //--------------------------------------------------------------------
        // Print POWER MODULE I_B voltage
        //--------------------------------------------------------------------
        if(isV2)
        {
            // MCP3426A6; SLAVE ADDRESS: 1101 110
            // Channel 1: I_A = 2.5 * V_CH1
            I_B = 2.5 * s20.readMezzADC(V2_PM_ADC, 2) / 1000;
        }
        else
        {
            // MCP3428; SLAVE ADDRESS: 1101 010
            // Channel 3: I_B = 2 * V_CH3
            // 2.048V reference -> 1 mV per LSB
            I_B = 2 * s20.readMezzADC(ADC_28, 3) / 1000;
        }
        if(!passive) fprintf(f_detail, "%7.3f A, ", I_B);

        //--------------------------------------------------------------------
        // Print POWER MODULE V_B current
        //--------------------------------------------------------------------
        if(isV2)
        {
            // 12 V input ADC_5 on sub20 (V_A = 9.2 * V_ADC5)
            if(s20.isRPi_) V_B = 9.2 * s20.readSUB20ADC(7 - 2*(aMuxAddr-1));
	    else           V_B = 9.2 * s20.readSUB20ADC(6);
        }
        else
        {
            // MCP3428; SLAVE ADDRESS: 1101 010
            // Channel 4: V_B = 18.647 * V_CH4
            // 2.048V reference -> 1 mV per LSB
            V_B = 18.647 * s20.readMezzADC(ADC_28, 4) / 1000;
        }
        if(!passive) fprintf(f_detail, "%7.3f V, ", V_B);

        //--------------------------------------------------------------------
        // Print POWER MODULE total power:  (I_A * V_A) + (I_B * V_B)
        //--------------------------------------------------------------------
        if(!passive) fprintf(f_detail, "%7.3f W\n", (I_A * V_A) + (I_B * V_B));

        if(!passive) fflush(f_detail);

        //--------------------------------------------------------------------
        // Check POWER MODULE balance.
        //--------------------------------------------------------------------
        if ((I_A / (I_A + I_B)) < 0.40)
        {
            actTest->pass |= RETVAL_BAD_POWERBALANCE;
        }
        if ((I_B / (I_A + I_B)) < 0.40)
        {
            actTest->pass |= RETVAL_BAD_POWERBALANCE;
        }
    }
    else
    {
        actTest->pass |= RETVAL_NO_MODLE_DETECTED;
        if(isMaybeNotThere)
        {
            setRun(false);
            setPrimaryLoad(false, false);
            isNotThere = true;
        }
        isMaybeNotThere = true;
    }

    //--------------------------------------------------------------------
    // SAFTEY CHECK
    //--------------------------------------------------------------------
    //No power mezzanine should draw more than 38 W total power
    if(I_A * V_A + I_B * V_B > 45.0) actTest->pass |= RETVAL_ABORT;
    //In temp is above 90 C then something is probably wrong
    if(p_temp > 90) actTest->pass |= RETVAL_ABORT;

    if(actTest->pass & RETVAL_ABORT)
    {
        setRun(false);
        setPrimaryLoad(false, false);
    }

    //--------------------------------------------------------------------
    // SUMMARY
    //--------------------------------------------------------------------
    actTest->setArray(actTest->temp, p_temp);
    actTest->setArray(actTest->vout, p_vout);
    actTest->setArray(actTest->V_A, V_A);
    actTest->setArray(actTest->V_B, V_B);
    actTest->setArray(actTest->I_A, I_A);
    actTest->setArray(actTest->I_B, I_B);
    actTest->setArray(actTest->P, V_A * I_A + V_B * I_B);

    passed();

    return actTest->pass;
}

unsigned int APM::monitor(bool passive)
{
    double a_temp = 0.0, a_vout = 0.0;
    double I_C = 0.0, V_C = 0.0;
    double VADJ_A = 0.0, VADJ_B = 0.0, VADJ_C = 0.0, VADJ_D = 0.0;
    bool a_margup, a_margdn, a_pgood;

    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);

    //--------------------------------------------------------------------
    // Print AUX_POWER MODULE MAC address
    //--------------------------------------------------------------------
    // 24AA02E48T; SLAVE ADDRESS: 1010 000
    if(!s20.readMezzMAC())
    {
        s20.copyMezzMAC(actTest->MAC);

        if(f_detail == NULL && !passive)
        {
            char fname[128];
            sprintf(fname, "AuxPowerMezz-%02x-%02x-%02x-%02x-%02x-%02x.txt", actTest->MAC[0]&0xff, actTest->MAC[1]&0xff, actTest->MAC[2]&0xff, actTest->MAC[3]&0xff, actTest->MAC[4]&0xff, actTest->MAC[5]&0xff);
            f_detail = fopen(fname, "w");
            fprintf(f_detail, "%25s, %17s, %7s, %9s, %5s, %7s, %7s, %5s, %9s, %9s, %9s, %9s, %9s, %9s, %9s\n", "Date", "MAC", "temp", "Vout", "PGOOD", "MARG_UP", "MARG_DN", "Load", "I_C", "V_C", "VADJ_A", "VADJ_B", "VADJ_C", "VADJ_D", "Total Power");
        }

        if(!passive)
        {
            // Print Timestamp
            time_t epch = time(0);
            char ts[128];
            int i = 0;
            sprintf(ts, "%s", asctime(gmtime(&epch)));
            while(ts[i] != '\n') i++;
            ts[i] = '\0';
            fprintf(f_detail, "%25s, ", ts);
            // Print MAC
            fprintf(f_detail, "%02x:%02x:%02x:%02x:%02x:%02x, ", actTest->MAC[0]&0xff, actTest->MAC[1]&0xff, actTest->MAC[2]&0xff, actTest->MAC[3]&0xff, actTest->MAC[4]&0xff, actTest->MAC[5]&0xff);
        }


        //--------------------------------------------------------------------
        // Print AUX_POWER MODULE temperature
        //--------------------------------------------------------------------
        // MCP3426A0; SLAVE ADDRESS: 1101 000
        // Channel 1: TEMP(degrees C) = 25 + (V_CH1 - 750) / 10
        // 2.048V reference -> 1 mV per LSB
        if(isV2) a_temp = 25 + (s20.readMezzADC(V2_MEZZ_ADC, 1) - 750) / 10;
        else     a_temp = 25 + (s20.readMezzADC(ADC_26, 1) - 750) / 10;
        if(!passive) fprintf(f_detail, "%5.1f C, ", a_temp);

        //--------------------------------------------------------------------
        // Print AUX_POWER MODULE V_OUT voltage
        //--------------------------------------------------------------------
        // MCP3426A0; SLAVE ADDRESS: 1101 000
        // Channel 2: V_OUT = 4.322 * V_CH2
        // 2.048V reference -> 1 mV per LSB
        if(isV2) a_vout = 4.322 * s20.readMezzADC(V2_MEZZ_ADC, 2) / 1000;
        else     a_vout = 4.322 * s20.readMezzADC(ADC_26, 2) / 1000;
        if(!passive) fprintf(f_detail, "%7.3f V, ", a_vout);

        //--------------------------------------------------------------------
        // Print AUX_POWER MODULE PGOOD status
        // Print AUX_POWER MODULE MARG_UP status.
        // Print AUX_POWER MODULE MARG_DWN status.
        //--------------------------------------------------------------------
        // PCA9534; SLAVE ADDRESS: 0100 000
        // Channel 7: PGOOD
        // Channel 2: MARG_UP
        // Channel 3: MARG_DWN
        s20.readMarginPGood(&a_margup, &a_margdn, &a_pgood);
        if(!passive) fprintf(f_detail, "%5d, %7d, %7d, %5d, ", a_pgood, a_margup, a_margdn, load);

        //--------------------------------------------------------------------
        // Print I_C voltage.
        //--------------------------------------------------------------------

        if(isV2)
        {
            // ADC128D818; SLAVE ADDRESS: 0011101
            // Channel 4: I_C = 1.5625 * V_CH4
            // 2.56 V reference -> 1.6 mV per LSB
            I_C = 1.5625 * s20.readMezzADC(V2_APM_ADC, 4) / 1000;
        }
        else
        {
            // MCP3428; SLAVE ADDRESS: 1101 010
            // Channel 1: I_A = 2 * V_CH1
            // 2.048V reference -> 1 mV per LSB
            I_C = 2 * s20.readMezzADC(ADC_28, 1) / 1000;
        }
        if(!passive) fprintf(f_detail, "%7.3f A, ", I_C);

        //--------------------------------------------------------------------
        // Print V_C voltage.
        //--------------------------------------------------------------------
        if(isV2)
        {
            // 12 V input ADC_5 on sub20 (V_A = 6 * V_ADC5)
            if(s20.isRPi_) V_C = 9.2 * s20.readSUB20ADC(7 - 2*(aMuxAddr-1));
	    else           V_C = 9.2 * s20.readSUB20ADC(6);
        }
        else
        {
            // MCP3428; SLAVE ADDRESS: 1101 010
            // Channel 2: V_A = 18.647 * V_CH2
            // 2.048V reference -> 1 mV per LSB
            V_C = 18.647 * s20.readMezzADC(ADC_28, 2) / 1000;
        }
        if(!passive) fprintf(f_detail, "%7.3f V, ", V_C);

        //--------------------------------------------------------------------
        // Print Vadj voltages.
        //--------------------------------------------------------------------
        if(isV2)
        {
            // ADC128D818; SLAVE ADDRESS: 0011101
            // Channel 0-3: V_Adj(A-D) = V_CH(0-3)
            // 2.56 V reference -> 1.6 mV per LSB
            VADJ_A = s20.readMezzADC(V2_APM_ADC, 0) / 1600.0;
            VADJ_B = s20.readMezzADC(V2_APM_ADC, 1) / 1600.0;
            VADJ_C = s20.readMezzADC(V2_APM_ADC, 2) / 1600.0;
            VADJ_D = s20.readMezzADC(V2_APM_ADC, 3) / 1600.0;
        }
        else
        {
            // SUB-20 ADC
            // Channel 0: VADJ_A = 2 * V_CH0
            // Channel 2: VADJ_B = 2 * V_CH2
            // Channel 4: VADJ_C = 2 * V_CH4
            // Channel 6: VADJ_D = 2 * V_CH6
            VADJ_A = 2 * s20.readSUB20ADC(0);
            VADJ_B = 2 * s20.readSUB20ADC(2);
            VADJ_C = 2 * s20.readSUB20ADC(4);
            VADJ_D = 2 * s20.readSUB20ADC(6);
        }
        if(!passive) fprintf(f_detail, "%7.3f V, %7.3f V, %7.3f V, %7.3f V, ", VADJ_A, VADJ_B, VADJ_C, VADJ_D);

        //--------------------------------------------------------------------
        // Print AUX_POWER MODULE total power:  (I_C * V_C)
        //-------------------------------------------------------------------- 
        if(!passive) fprintf(f_detail, "%7.3f W\n", I_C * V_C);

        if(!passive) fflush(f_detail);
    }
    else
    {
        actTest->pass |= RETVAL_NO_MODLE_DETECTED;

        if(isMaybeNotThere)
        {
            setRun(false);
            setPrimaryLoad(false, false);
            setSecondaryLoad(false, false, false, false);
            isNotThere = true;
        }
        isMaybeNotThere = true;
    }

    //--------------------------------------------------------------------
    // SAFTEY CHECK
    //--------------------------------------------------------------------
    //No aux power mezzanine should draw more than 38 W total power
    if(I_C * V_C > 38.0) actTest->pass |= RETVAL_ABORT;
    //If temp is above 90 C then something is probably wrong
    if(a_temp > 90) actTest->pass |= RETVAL_ABORT;

    if(actTest->pass & RETVAL_ABORT)
    {
        setRun(false);
        setPrimaryLoad(false, false);
        setSecondaryLoad(false, false, false, false);
    }

    //--------------------------------------------------------------------
    // SUMMARY
    //--------------------------------------------------------------------
    actTest->setArray(actTest->temp, a_temp);
    actTest->setArray(actTest->vout, a_vout);
    actTest->setArray(actTest->I_C, I_C);
    actTest->setArray(actTest->Vadj_A, VADJ_A);
    actTest->setArray(actTest->Vadj_B, VADJ_B);
    actTest->setArray(actTest->Vadj_C, VADJ_C);
    actTest->setArray(actTest->Vadj_D, VADJ_D);
    actTest->setArray(actTest->P, V_C * I_C);

    passed();

    return actTest->pass;
}

void PM::setPrimaryLoad(const bool p, const bool s)
{
    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);

    if(isV2)
    {
        s20.toggleMezzsLoad(1, p);
        s20.toggleMezzsLoad(2, s);
    }
}

void APM::setPrimaryLoad(const bool p, const bool s)
{
    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);

    if(isV2)
    {
        s20.toggleMezzsLoad(5, p);
        s20.toggleMezzsLoad(6, s);
    }
}

void APM::setSecondaryLoad(const bool l1, const bool l2, const bool l3, const bool l4)
{
    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);

    if(isV2)
    {
        s20.toggleMezzsLoad(1, l1);
        s20.toggleMezzsLoad(2, l2);
        s20.toggleMezzsLoad(3, l3);
        s20.toggleMezzsLoad(4, l4);
    }
}

bool PM::programEeprom(std::string tester, std::string site)
{
    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);
    //activate the power mezzanine so the eeprom is powered
    setRun(true);
    usleep(100000); // Sleep to ensure mezzanine is fully powered

    bool isError = false;

    // Get time
    time_t dummy_time;
    tm *ct = NULL;
    time(&dummy_time);
    ct = gmtime(&dummy_time);

    if(s20.readMezzMAC()) printf("!!!PM not detected for labeling!!!\n");
    else
    {
        // Make and populate data structure
        uHTRPowerMezzInterface::EEPROM_data data;
        // Initialize the structure to 0
        for(unsigned int i = 0; i < uHTRPowerMezzInterface::EEPROM_DATA_SIZE; i++) *(((uint8_t*) & data) + i) = 0;
        data.data_format_version = 0x01;
        data.mezz_type_code = 0x01;
        mezzLabel ml = (mezzLabel)900;
        if(VOUT_NOM > 0.99 && VOUT_NOM < 1.01)
        {
            data.mezz_subtype_code = 0x01;
            sprintf((char*)data.mezz_type, "PM1.0");
            ml = PM1_0;
        }
        else if(VOUT_NOM > 3.29 && VOUT_NOM < 3.31)
        {
            data.mezz_subtype_code = 0x03;
            sprintf((char*)data.mezz_type, "PM3.3");
            ml = PM3_3;
        }
        else
        {
            data.mezz_subtype_code = 0xff;
            sprintf((char*)data.mezz_type, "UNKNOWN PM");
        }
        //Get serial number
        char cmac[6];
        s20.copyMezzMAC(cmac);
        unsigned int sn = getSN(cmac, ml);
        data.serial_number[0] = 0x00ff & sn;
        data.serial_number[1] = ((0xff00 & sn) >> 8);
        sprintf((char*)data.manu_date, "%d-%d-%d", ct->tm_mday, ct->tm_mon + 1, ct->tm_year + 1900);
        sprintf((char*)data.manu_site, "%s", site.c_str());
        sprintf((char*)data.manu_tester, "%s", tester.c_str());
        sprintf((char*)data.test_release, "%d.%d.%d", uPMTV_MAJOR, uPMTV_MINOR, uPMTV_PATCH);

        // Write data structure to eeprom
        s20.writeMezzEeprom(data);
        if(s20.getError())
        {
            printf("!!!PM eeprom i2c write error!!!\n");
            s20.updateSUB20Display("\fPM EPROM\nw failed");
            isError = true;
        }
        else
        {
            // Verify eeprom write
            uHTRPowerMezzInterface::EEPROM_data rb_data;
            s20.readMezzEeprom(&rb_data);
            for(unsigned int i = 0; i < uHTRPowerMezzInterface::EEPROM_DATA_SIZE; i++) isError |= (*((uint8_t*) & data + i) != *((uint8_t*) & rb_data + i));
            if(isError)
            {
                printf("!!!PM EEPROM verify failed!!!\n");
                s20.updateSUB20Display("\fPM EPROM\nw failed");
            }
            else
            {
                printf("PM EEPROM write successful\n");
                s20.updateSUB20Display("\fPM EPROM\nwrite OK");
            }
        }
    }

    //deactivate power mezzanines
    setRun(false);

    return !isError;
}

bool APM::programEeprom(std::string tester, std::string site)
{
    // PCA9544A; SLAVE ADDRESS: 1110 000
    s20.setMUXChannel(muxAddr, aMuxAddr);
    //activate the power mezzanine so the eeprom is powered
    setRun(true);
    usleep(100000); // Sleep to ensure mezzanine is fully powered

    // Get time
    time_t dummy_time;
    tm *ct = NULL;
    time(&dummy_time);
    ct = gmtime(&dummy_time);

    bool isError = false;

    if(s20.readMezzMAC()) printf("!!!APM not detected for labeling!!!\n");
    else
    {
        // Make and populate data structure
        uHTRPowerMezzInterface::EEPROM_data data;
        // Initialize the structure to 0
        for(unsigned int i = 0; i < uHTRPowerMezzInterface::EEPROM_DATA_SIZE; i++) *(((uint8_t*) & data) + i) = 0;
        data.data_format_version = 0x01;
        data.mezz_type_code = 0x02;
        mezzLabel ml = (mezzLabel)900;
        if(VOUT_NOM > 1.79 && VOUT_NOM < 1.81)
        {
            data.mezz_subtype_code = 0x01;
            sprintf((char*)data.mezz_type, "APM1.8");
            ml = APM1_8;
        }
        else if(VOUT_NOM > 2.49 && VOUT_NOM < 2.51)
        {
            data.mezz_subtype_code = 0x02;
            sprintf((char*)data.mezz_type, "APM2.5");
            ml = APM2_5;
        }
        else
        {
            data.mezz_subtype_code = 0xff;
            sprintf((char*)data.mezz_type, "UNKNOWN APM");
        }
        //Get serial number
        char cmac[6];
        s20.copyMezzMAC(cmac);
        unsigned int sn = getSN(cmac, ml);
        data.serial_number[0] = 0x00ff & sn;
        data.serial_number[1] = ((0xff00 & sn) >> 8);
        sprintf((char*)data.manu_date, "%d-%d-%d", ct->tm_mday, ct->tm_mon + 1, ct->tm_year + 1900);
        sprintf((char*)data.manu_site, "%s", site.c_str());
        sprintf((char*)data.manu_tester, "%s", tester.c_str());
        sprintf((char*)data.test_release, "%d.%d.%d", uPMTV_MAJOR, uPMTV_MINOR, uPMTV_PATCH);

        // Write data structure to eeprom
        s20.writeMezzEeprom(data);
        if(s20.getError())
        {
            printf("!!!APM eeprom i2c write error!!!\n");
            s20.updateSUB20Display("\fAM EPROM\nw failed");
            isError = true;
        }
        else
        {
            // Verify eeprom write
            uHTRPowerMezzInterface::EEPROM_data rb_data;
            s20.readMezzEeprom(&rb_data);
            for(unsigned int i = 0; i < uHTRPowerMezzInterface::EEPROM_DATA_SIZE; i++) isError |= (*((uint8_t*) & data + i) != *((uint8_t*) & rb_data + i));
            if(isError)
            {
                printf("!!!APM EEPROM verify failed!!!\n");
                s20.updateSUB20Display("\fAM EPROM\nw failed");
            }
            else
            {
                printf("APM EEPROM write successful\n");
                s20.updateSUB20Display("\fAM EPROM\nwrite OK");
            }
        }

    }

    //deactivate power mezzanines
    setRun(false);

    return !isError;
}

void PM::print()
{
    if(actTest->pass & RETVAL_NO_MODLE_DETECTED) return;

    //open the pm summary file if it is not already open
    f_pm_sum = fopen("PowerMezzSummary.txt", "a");

    passed();

    // Print Timestamp
    time_t epch = time(0);
    char ts[128];
    int i = 0;
    sprintf(ts, "%s", asctime(gmtime(&epch)));
    while(ts[i] != '\n') i++;
    ts[i] = '\0';

    if(margin < 0)      fprintf(f_pm_sum, "Margin up,  ");
    else if(margin > 0) fprintf(f_pm_sum, "Margin dn,  ");
    else
        if(load == 2)   fprintf(f_pm_sum, "Hl Nominal, ");
    else            fprintf(f_pm_sum, "Nominal,    ");

    char error[128];
    Summary::translateStatus(actTest->pass, error);
    fprintf(f_pm_sum, "%02x:%02x:%02x:%02x:%02x:%02x, %8s, %0.2f V, %0.2f V, %0.2f V, %s\n", actTest->MAC[0]&0xFF, actTest->MAC[1]&0xFF, actTest->MAC[2]&0xFF, actTest->MAC[3]&0xFF, actTest->MAC[4]&0xFF, actTest->MAC[5]&0xFF,
            (actTest->pass == RETVAL_SUCCESS)?"PASS":error, actTest->vout[0], actTest->vout[1] / actTest->vout[3], actTest->vout[2], ts);
    fclose(f_pm_sum);
}

bool PM::init()
{
    if(!isInit)
    {
        int error = s20.setMUXChannel(muxAddr, aMuxAddr);
        if (error) return false;

        s20.configGPIO();
        isInit = true;
    }
    return isInit;
}

bool APM::init()
{
    if(!isInit)
    {
        int error = s20.setMUXChannel(muxAddr, aMuxAddr);
        if(error) 
        {
            return false;
        }
        s20.configADC128();

        s20.configGPIO();
        isInit = true;
    }
    return isInit;
}

void APM::print()
{
    if(actTest->pass & RETVAL_NO_MODLE_DETECTED) return;

    //open the apm summary file if it is not already open
    f_apm_sum = fopen("AuxPowerMezzSummary.txt", "a");

    passed();

    // Print Timestamp
    time_t epch = time(0);
    char ts[128];
    int i = 0;
    sprintf(ts, "%s", asctime(gmtime(&epch)));
    while(ts[i] != '\n') i++;
    ts[i] = '\0';

    if(margin < 0)      fprintf(f_apm_sum, "Margin up, ");
    else if(margin > 0) fprintf(f_apm_sum, "Margin dn, ");
    else
        if(load == 2)   fprintf(f_apm_sum, "Hl Nominal, ");
    else            fprintf(f_apm_sum, "Nominal,    ");

    char error[128];
    Summary::translateStatus(actTest->pass, error);
    fprintf(f_apm_sum, "%02x:%02x:%02x:%02x:%02x:%02x, %8s, %0.2f V, %0.2f V, %0.2f V, %s\n", actTest->MAC[0]&0xFF, actTest->MAC[1]&0xFF, actTest->MAC[2]&0xFF, actTest->MAC[3]&0xFF, actTest->MAC[4]&0xFF, actTest->MAC[5]&0xFF,
            (actTest->pass == RETVAL_SUCCESS)?"PASS":error, actTest->vout[0], actTest->vout[1] / actTest->vout[3], actTest->vout[2], ts);
    fclose(f_apm_sum);
}

bool PM::passed()
{
    if(actTest->vout[3] < 1.5) return true;
    if(actTest->vout[1] / actTest->vout[3] < 0.9 * VOUT_NOM * (1 + actTest->marg_state) || actTest->vout[1] / actTest->vout[3] > 1.1 * VOUT_NOM * (1 + actTest->marg_state)) actTest->pass |= RETVAL_BAD_VOUT;
    if(actTest->vout[0] < 0.8 * VOUT_NOM * (1 + actTest->marg_state) || actTest->vout[2] > 1.2 * VOUT_NOM * (1 + actTest->marg_state)) actTest->pass |= RETVAL_BAD_VOUT;

    if(actTest->V_A[0] < 0.7 * actTest->V_A[1] / actTest->V_A[3] || actTest->V_A[2] > 1.3 * actTest->V_A[1] / actTest->V_A[3]) actTest->pass |= RETVAL_BAD_VA;
    if(actTest->V_B[0] < 0.7 * actTest->V_B[1] / actTest->V_B[3] || actTest->V_B[2] > 1.3 * actTest->V_B[1] / actTest->V_B[3]) actTest->pass |= RETVAL_BAD_VB;
    if(actTest->I_A[0] < 0.7 * actTest->I_A[1] / actTest->I_A[3] || actTest->I_A[2] > 1.3 * actTest->I_A[1] / actTest->I_A[3]) actTest->pass |= RETVAL_BAD_IA;
    if(actTest->I_B[0] < 0.7 * actTest->I_B[1] / actTest->I_B[3] || actTest->I_B[2] > 1.3 * actTest->I_B[1] / actTest->I_B[3]) actTest->pass |= RETVAL_BAD_IB;

    if(actTest->P[0] < 0.7 * actTest->P[1] / actTest->P[3] || actTest->P[2] > 1.3 * actTest->P[1] / actTest->P[3]) actTest->pass |= RETVAL_BAD_POWER;

    return (bool(actTest->pass == RETVAL_SUCCESS));
}

bool APM::passed()
{
    if(actTest->vout[3] < 1.5) return true;
    if(actTest->vout[1] / actTest->vout[3] < 0.9 * VOUT_NOM * (1 + actTest->marg_state) || actTest->vout[1] / actTest->vout[3] > 1.1 * VOUT_NOM * (1 + actTest->marg_state)) actTest->pass |= RETVAL_BAD_VOUT;
    if(actTest->vout[0] < 0.8 * VOUT_NOM * (1 + actTest->marg_state) || actTest->vout[2] > 1.2 * VOUT_NOM * (1 + actTest->marg_state)) actTest->pass |= RETVAL_BAD_VOUT;

    if(actTest->V_C[0] < 0.7 * actTest->V_C[1] / actTest->V_C[3] || actTest->V_C[2] > 1.3 * actTest->V_C[1] / actTest->V_C[3]) actTest->pass |= RETVAL_BAD_VC;
    if(actTest->I_C[0] < 0.7 * actTest->I_C[1] / actTest->I_C[3] || actTest->I_C[2] > 1.3 * actTest->I_C[1] / actTest->I_C[3]) actTest->pass |= RETVAL_BAD_IC;

    if(actTest->P[0] < 0.7 * actTest->P[1] / actTest->P[3] || actTest->P[2] > 1.3 * actTest->P[1] / actTest->P[3]) actTest->pass |= RETVAL_BAD_POWER;

    if(actTest->Vadj_A[1] / actTest->Vadj_A[3] < 0.9 * APM_VA_VB_NOM * (1 + actTest->marg_state) || actTest->Vadj_A[1] / actTest->Vadj_A[3] > 1.1 * APM_VA_VB_NOM * (1 + actTest->marg_state)) actTest->pass |= RETVAL_BAD_VADJ;
    if(actTest->Vadj_A[0] < 0.8 * APM_VA_VB_NOM * (1 + actTest->marg_state) || actTest->Vadj_A[2] > 1.2 * APM_VA_VB_NOM * (1 + actTest->marg_state)) actTest->pass |= RETVAL_BAD_VADJ;

    if(actTest->Vadj_C[1] / actTest->Vadj_C[3] < 0.9 * APM_VC_VD_NOM * (1 + actTest->marg_state) || actTest->Vadj_C[1] / actTest->Vadj_C[3] > 1.1 * APM_VC_VD_NOM * (1 + actTest->marg_state)) actTest->pass |= RETVAL_BAD_VADJ;
    if(actTest->Vadj_C[0] < 0.8 * APM_VC_VD_NOM * (1 + actTest->marg_state) || actTest->Vadj_C[2] > 1.2 * APM_VC_VD_NOM * (1 + actTest->marg_state)) actTest->pass |= RETVAL_BAD_VADJ;

    return (bool(actTest->pass == RETVAL_SUCCESS));
}

unsigned int Mezzanines::monitor(bool passive)
{
    int status = 0;
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM)
    {
        boost::mutex::scoped_lock l(*s20mtx_);
        if(!(*iM)->isPresent()) continue;
        status |= (*iM)->monitor(passive);
    }
    return status;

}
void Mezzanines::setMargins(const int margin, const int l)
{
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM)
    {
        boost::mutex::scoped_lock lock(*s20mtx_);
        if(!(*iM)->isPresent()) continue;
        (*iM)->setMargins(margin,l);
    }
}
void Mezzanines::setRun(const bool run)
{
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM)
    {
        boost::mutex::scoped_lock l(*s20mtx_);
        if(!(*iM)->isPresent()) continue;
        (*iM)->setRun(run);
    }
}
void Mezzanines::setPrimaryLoad(const bool p, const bool s)
{
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM)
    {
        boost::mutex::scoped_lock l(*s20mtx_);
        if(!(*iM)->isPresent()) continue;
        (*iM)->setPrimaryLoad(p,s);
    }

}
void Mezzanines::setSecondaryLoad(const bool l1, const bool l2, const bool l3, const bool l4)
{
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM)
    {
        boost::mutex::scoped_lock l(*s20mtx_);
        if(!(*iM)->isPresent()) continue;
        if(!(*iM)->isPM) (*iM)->setSecondaryLoad(l1,l2,l3,l4);
    }
}
bool Mezzanines::labelPM(const std::string tester, const std::string site)
{
    bool ret = false;
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM) 
    {
        boost::mutex::scoped_lock l(*s20mtx_);
        if(!(*iM)->isPresent()) continue;
        if((*iM)->isPM) 
            ret |= (*iM)->programEeprom(tester, site);
    }
    return ret;
}
bool Mezzanines::labelAPM(const std::string tester, const std::string site)
{
    bool ret = false;
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM) 
    {
        boost::mutex::scoped_lock l(*s20mtx_);
        if(!(*iM)->isPresent()) continue;
        if(!(*iM)->isPM) 
            ret |= (*iM)->programEeprom(tester, site);
    }
    return ret;
}
void Mezzanines::readEeprom()
{
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM)
    {
        boost::mutex::scoped_lock l(*s20mtx_);
        if(!(*iM)->isPresent()) continue;
        (*iM)->readEeprom();
    }

}

int Mezzanines::init()
{
    int isInit = 1;
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM)
    {
        boost::mutex::scoped_lock l(*s20mtx_);
        isInit &= (*iM)->init();
    }
    return isInit;
}

void Mezzanines::print()
{
    std::vector<Mezzanine*>::iterator iM;
    for(iM = begin(); iM != end(); ++iM)
    {
        boost::mutex::scoped_lock l(*s20mtx_);
        (*iM)->print();
    }

}

void Mezzanines::displayAndSleep(uHTRPowerMezzInterface& s20)
{
    char displaybuff[20], error[10];

    std::vector<Mezzanine*>::iterator iM;
    std::vector<Mezzanine*>::iterator iMBegin =begin();
    std::vector<Mezzanine*>::iterator iMEnd = end();

    while(true)
    {
        for(iM = iMBegin; iM != iMEnd; ++iM)
        {
            if(!(*iM)->isPresent()) continue;
            {
                boost::mutex::scoped_lock l2(*s20mtx_);
                if((*iM)->isPM) sprintf(displaybuff, "\fPM %0.1fV\nT: %0.1fC", (*iM)->VOUT_NOM, (*iM)->actTest->temp[4]);
                else            sprintf(displaybuff, "\fAPM %0.1fV\nT: %0.1fC", (*iM)->VOUT_NOM, (*iM)->actTest->temp[4]);
                s20.updateSUB20Display(displaybuff);
            }
            sleep(2);
            {
                boost::mutex::scoped_lock l2(*s20mtx_);
                if((*iM)->isPM) sprintf(displaybuff, "\fPM %0.1fV\nV: %0.2fV", (*iM)->VOUT_NOM, (*iM)->actTest->vout[4]);
                else            sprintf(displaybuff, "\fAPM %0.1fV\nV: %0.2fV", (*iM)->VOUT_NOM, (*iM)->actTest->vout[4]);
                s20.updateSUB20Display(displaybuff);
            }
            sleep(2);
            {
                boost::mutex::scoped_lock l2(*s20mtx_);
                Mezzanine::Summary::translateStatus((*iM)->actTest->pass, error);
                if((*iM)->isPM) sprintf(displaybuff, "\fPM %0.1fV\n%s", (*iM)->VOUT_NOM, error);
                else            sprintf(displaybuff, "\fAPM %0.1fV\n%s", (*iM)->VOUT_NOM, error);
                s20.updateSUB20Display(displaybuff);
            }
            sleep(2);
        }
    }
}
