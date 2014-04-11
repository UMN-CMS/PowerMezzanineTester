#ifndef UHTRMEZZANINES_H
#define UHTRMEZZANINES_H

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <time.h>
#include <map>
#include <string>
#include "boost/thread.hpp"
#include "uHTRPowerMezzInterface.h"

// Software Version 
const static unsigned int uPMTV_MAJOR = 1;
const static unsigned int uPMTV_MINOR = 0;
const static unsigned int uPMTV_PATCH = 2;

const double APM_VA_VB_NOM = 1.2;
const double APM_VC_VD_NOM = 1.0;

const double MARGIN_OFFSET = 0.05; //percent above/below nominal 

enum mezzLabel
{
    PM1_0, PM3_3, APM1_8, APM2_5
};

enum ADCSwitch
{
    ADC_28 = 0,
    ADC_26 = 1
};

enum V2_ADCSwitch
{
    V2_MEZZ_ADC = 0,
    V2_PM_ADC   = 1,
    V2_APM_ADC  = 2
};

enum testRetVals
{
    RETVAL_SUCCESS = 0x0000,
    RETVAL_ABORT = 0x0001,
    RETVAL_BAD_SUPPLY_VOLTAGE = 0x0002,
    RETVAL_BAD_3_3VOLT = 0x0004,
    RETVAL_BAD_POWERBALANCE = 0x0008,
    RETVAL_BAD_VOUT = 0x0010,
    RETVAL_BAD_VA = 0x0020,
    RETVAL_BAD_VB = 0x0040,
    RETVAL_BAD_VC = 0x0080,
    RETVAL_BAD_IA = 0x0100,
    RETVAL_BAD_IB = 0x0200,
    RETVAL_BAD_IC = 0x0400,
    RETVAL_BAD_VADJ = 0x0800,
    RETVAL_BAD_POWER = 0x1000,
    RETVAL_NO_MODLE_DETECTED = 0x2000,
    RETVAL_NO_MEZZ_SPEC= 0x4000
};

class Mezzanine
{
public:

    struct Summary
    {
        char MAC[6];
        double temp[5]; //min, avg, max, N, last
        double vout[5];
        double V_A[5], V_B[5], V_C[5], I_A[5], I_B[5], I_C[5];
        double P[5];
        double Vadj_A[5], Vadj_B[5], Vadj_C[5], Vadj_D[5];
        int pass;
        double marg_state;

        void setArray(double* array, double val)
        {
            if(val < array[0]) array[0] = val;
            if(val > array[2]) array[2] = val;
            array[1] += val;
            array[3] += 1.0;
            array[4] = val;
        }

        Summary()
        {
            temp[0] = 999999999.0;
            vout[0] = 999999999.0;
            V_A[0] = V_B[0] = V_C[0] = 999999999.0;
            I_A[0] = I_B[0] = I_C[0] = 999999999.0;
            P[0] = 999999999.0;
            Vadj_A[0] = Vadj_B[0] = Vadj_C[0] = Vadj_D[0] = 999999999.0;
            for(int i = 1; i < 5; i++)
            {
                temp[i] = 0.0;
                vout[i] = 0.0;
                V_A[i] = V_B[i] = V_C[i] = 0.0;
                I_A[i] = I_B[i] = I_C[i] = 0.0;
                P[i] = 0.0;
                Vadj_A[i] = Vadj_B[i] = Vadj_C[i] = Vadj_D[i] = 0.0;
            }
            pass = RETVAL_SUCCESS;
            marg_state = 0;
        }

        static void translateStatus(const unsigned int status, char *buff);
    };

protected:

    uHTRPowerMezzInterface& s20;
    const int muxAddr, aMuxAddr;
    Summary margUp, margDn, margNom, hlNom;
    int margin, load;
    bool isV2, isMaybeNotThere, isNotThere;

    FILE *f_detail;

    static std::map<std::string, std::pair<unsigned int, unsigned int> > snList;
    static unsigned int sn_max[4];

    void loadSSNFile(const unsigned int skipBlockSize);
    unsigned int getSN(const char * const cmac, const mezzLabel);

public:

    Summary *actTest;    
    const bool isPM;
    const double VOUT_NOM;

    Mezzanine(uHTRPowerMezzInterface& s, const int muxAddress, const double vout, const bool ispm, bool V2, int aMuxAdd = -1);
    virtual ~Mezzanine();

    virtual unsigned int monitor() = 0;
    virtual void setMargins(const int margin, const int l = 1);
    virtual void setRun(const bool run);
    virtual void setPrimaryLoad(const bool p, const bool s) = 0;
    virtual void setSecondaryLoad(const bool l1, const bool l2, const bool l3, const bool l4){ }
    virtual bool programEeprom(const std::string tester, const std::string site) = 0;
    virtual void readEeprom();
    virtual bool isPresent()
    {
        return !isNotThere;
    }
    int get_adChan() { return aMuxAddr; }

    virtual void print() = 0;
    virtual bool passed() = 0;
};

class PM : public Mezzanine
{
private:
    FILE *f_pm_sum;
public:

PM(uHTRPowerMezzInterface& s, const int muxAddress, const double vout, bool isV2, int aMuxAddr = -1) : Mezzanine(s, muxAddress, vout, true, isV2, aMuxAddr)
    {
    }
    unsigned int monitor();
    void setPrimaryLoad(const bool p, const bool s);
    bool programEeprom(const std::string tester, const std::string site);
    void print();
    bool passed();
};

class APM : public Mezzanine
{
private:
    FILE *f_apm_sum;
public:

APM(uHTRPowerMezzInterface& s, const int muxAddress, const double vout, bool isV2, int aMuxAddr = -1) : Mezzanine(s, muxAddress, vout, false, isV2, aMuxAddr)
    {
        s.setMUXChannel(muxAddress, aMuxAddr);
        s.configADC128();
    }
    unsigned int monitor();
    void setPrimaryLoad(const bool p, const bool s);
    void setSecondaryLoad(const bool l1, const bool l2, const bool l3, const bool l4);
    bool programEeprom(const std::string tester, const std::string site);
    void print();
    bool passed();
};

class Mezzanines : public std::vector<Mezzanine*>
{
    public:
        static Mezzanines * Instance();
        unsigned int monitor();
        void setMargins(const int margin, const int l = 1);
        void setRun(const bool run);
        void setPrimaryLoad(const bool p, const bool s);
        void setSecondaryLoad(const bool l1, const bool l2, const bool l3, const bool l4);
        bool labelPM(const std::string tester, const std::string site);
        bool labelAPM(const std::string tester, const std::string site);
        void readEeprom();


        void print();
        void displayAndSleep(uHTRPowerMezzInterface& s20);

    private:
        Mezzanines() {}
        static Mezzanines * pM;
        boost::mutex * s20mtx_;
        
};
#endif
