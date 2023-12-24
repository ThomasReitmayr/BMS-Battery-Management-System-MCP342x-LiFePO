/*
Battery Management System for 4S LiFePO 

MCP342x:
  MCP 3424 version 1.9.3 example sketch Multichannel
  Written by B@tto 
  Contact : batto@hotmail.fr
  MCP342x.cpp - ADC 18 bits i2c library for Wiring & Arduino
  Copyright (c) 2012 Yann LEFEBVRE.  All right reserved.

U8g2lib:
   The U8g2lib code (http://code.google.com/p/u8g2/) is licensed under the terms of 
   the new-bsd license (two-clause bsd license).
   See also: http://www.opensource.org/licenses/bsd-license.php
   Fonts are licensed under different conditions.
   See 
	   https://github.com/olikraus/u8g2/wiki/fntgrp
   for detailed information on the licensing conditions for each font.
   The example code in sys/raspi_gpio/hal will use the bcm2835 lib from Mike McCauley
   which is licensed under GPL V3: http://www.airspayce.com/mikem/bcm2835/
*/

#include <Wire.h>
#include <MCP342x.h>
#include <U8g2lib.h>

// i2c_Address OLED is 0x3c 
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Floats for cell voltage
double voltage_cell_1  = 0.000;
double voltage_cell_2  = 0.000;
double voltage_cell_3  = 0.000;
double voltage_cell_4  = 0.000;
double voltage_battery = 0.000;
double voltage_divider_factor =  100000000.00000;    // Ratio of voltage divider (no need for calibration as resistor tolerance <0,1% --> use voltmeter to check) (R2/(R1+R2))
                                                     // if tolerance of resistors is out of your specs you need to apply and calibrate a factor for each cell individually 

String voltage_cell_1_string;                        // string for printing on OLED with u8g2lib
String voltage_cell_2_string;
String voltage_cell_3_string;
String voltage_cell_4_string;
String voltage_battery_string;

// Temporary storage of voltages per cell in order to control relays via pre defined thresholds
double check_voltage_cell_1 = 0.000;
double check_voltage_cell_2 = 0.000;
double check_voltage_cell_3 = 0.000;
double check_voltage_cell_4 = 0.000;

// Threshold voltages for relays
const float VMaxOFF = 3.650;                          // Used for switching off Charge-Relay based on the maximum cell voltage
const float VChaON  = 3.450;                          // Used for switching on Charge-Relay again after switching off due to VMaxOFF based on the maximum cell voltage
const float VMinOFF = 3.200;                          // Used for switching off Load-Relay based on the minimum cell voltage
const float VMinON  = 3.250;                          // Used for switching on Load-Relay based on the minimum cell voltage
const float VBalON  = 3.380;                          // Used for switching on Balancer-Relay based on the maximum cell voltage
const float VBalOFF = 3.340;                          // Used for switching off Balancer-Relay based on the maximum cell voltage
const float VHigh   = 3.750;                          // Used for emergency switch off of Charge and Load-Relay based on the maximum cell voltage - in case first switching threholds did not work
const float VLow    = 3.000;                          // Used for emergency switch off of Charge and Load-Relay based on the minimum cell voltage - in case first switching threholds did not work


// Parameters for relay OUTPUT digitalwrite()
const int BalanceRelay = 8;
const int ChargeRelay  = 9;
const int LoadRelay    = 10;

// StatusTextParameter
String BalanceRelayStatus;
String ChargeRelayStatus;
String LoadRelayStatus;

MCP342x MCP(0);  // Declaration of MCP3424
long Voltage[4]; // Array used to store results

//__________________________________________SETUP___________________________________________________
void setup(){

  // Setting pinMode for 3 relays

  pinMode (ChargeRelay, OUTPUT);
  pinMode (LoadRelay, OUTPUT);
  pinMode (BalanceRelay, OUTPUT);

Serial.begin(9600);  

// Setup Serial Monitor
u8g2.begin();
MCP.begin(0);

// 5 DUMMY measures as first measures are inaccurate
   for (int i = 0; i < 5; i++) {

    for(int v=1;v<=4;v++){

      MCP.setConfiguration(v,RESOLUTION_16_BITS,CONTINUOUS_MODE,PGA_X1); // MCP3424 is configured to channel i with 16 bits resolution, continuous mode and gain defined to 1 
      Voltage[v-1]=MCP.measure();                                        // Measure is stocked in array Voltage, note that the library will wait for a completed conversion that takes around 200 ms@18bits

delay(10);
  }

}

// Initializing relays

    //Measure voltage before Initializing
      voltage_cell_1 = (Voltage[0] / voltage_divider_factor);
      voltage_cell_2 = (Voltage[1] / voltage_divider_factor);
      voltage_cell_3 = (Voltage[2] / voltage_divider_factor);
      voltage_cell_4 = (Voltage[3] / voltage_divider_factor);


  // Determination of max-voltage and min-voltage of the 4 cells based on last DUMMY measure

  double MesswertArray [4] = {voltage_cell_1, voltage_cell_2, voltage_cell_3, voltage_cell_4};
  double MaxMesswertArray = MesswertArray[0];
  double MinMesswertArray = MesswertArray[0];

  for (int s = 0; s < 4; s++){
      if (MesswertArray[s] > MaxMesswertArray) 
        {MaxMesswertArray = MesswertArray[s];}
      
      if (MesswertArray[s] < MinMesswertArray) 
        {MinMesswertArray = MesswertArray[s];}
      
  }

  // Initializing  --  Switching of relays 

if (MaxMesswertArray >= VMaxOFF)                   // Priority Charge Relay is switched off
   {digitalWrite (ChargeRelay, LOW);               // ChargeRelay OFF
    ChargeRelayStatus = ("Cx");}
else                                               
     {digitalWrite (ChargeRelay, HIGH);            // ChargeRelay ON
      ChargeRelayStatus = ("C+");}

if (MaxMesswertArray >= VBalON)                    // Priority Balance Relay is switched on
    {digitalWrite (BalanceRelay, HIGH);            // BalanceRelay ON
     BalanceRelayStatus = ("B+");}
else                                              
     {digitalWrite (BalanceRelay, LOW);            // BalanceRelay OFF
      BalanceRelayStatus = ("Bx");}

if (MinMesswertArray <= VMinOFF)                   // Priority Load Relay is switched off
   {digitalWrite (LoadRelay, LOW);                 // LoadRelay OFF
    digitalWrite (BalanceRelay, LOW);              // BalanceRelay OFF
    LoadRelayStatus = ("Lx");
    BalanceRelayStatus = ("Bx");}
else
     {digitalWrite (LoadRelay, HIGH);              // LoadRelay ON    
      LoadRelayStatus = ("L+");}

}

//________________________________________LOOP_____________________________________________________

 void loop(){

   // Read input parameters
   // Calculate voltages
   
 for (int m = 0; m < 10; m++) {          // # measure cycles = approx. 3 seconds

    for(int v=1;v<=4;v++){

      MCP.setConfiguration(v,RESOLUTION_16_BITS,CONTINUOUS_MODE,PGA_X1); // MCP3424 is configured to channel i with 16 bits resolution, continuous mode and gain defined to 1 
      Voltage[v-1]=MCP.measure();                                        // Measure is stocked in array Voltage, note that the library will wait for a completed conversion that takes around 200 ms@18bits
  }

   // Measure voltage ADC input
      voltage_cell_1 = (Voltage[0] / voltage_divider_factor);
      voltage_cell_2 = (Voltage[1] / voltage_divider_factor); 
      voltage_cell_3 = (Voltage[2] / voltage_divider_factor); 
      voltage_cell_4 = (Voltage[3] / voltage_divider_factor); 
  
  // Sum voltage measure for callculation of average voltage per cell
      check_voltage_cell_1 = check_voltage_cell_1 + voltage_cell_1;
      check_voltage_cell_2 = check_voltage_cell_2 + voltage_cell_2;
      check_voltage_cell_3 = check_voltage_cell_3 + voltage_cell_3;
      check_voltage_cell_4 = check_voltage_cell_4 + voltage_cell_4;
  
}    // End volatge measure

// Calcualtion of average per cell
   check_voltage_cell_1 = check_voltage_cell_1 / 10.000;
   check_voltage_cell_2 = check_voltage_cell_2 / 10.000;
   check_voltage_cell_3 = check_voltage_cell_3 / 10.000;
   check_voltage_cell_4 = check_voltage_cell_4 / 10.000;
   voltage_battery      = check_voltage_cell_1 + check_voltage_cell_2 + check_voltage_cell_3 + check_voltage_cell_4;


// Determination of minimum and maximum cell voltage in the battery pack
   double MesswertArray [4] = {check_voltage_cell_1, check_voltage_cell_2, check_voltage_cell_3, check_voltage_cell_4};
   double MaxMesswertArray = MesswertArray[0];
   double MinMesswertArray = MesswertArray[0];

  for (int s = 0; s < 4; s++){
      if (MesswertArray[s] > MaxMesswertArray) 
        {MaxMesswertArray = MesswertArray[s];}
      if (MesswertArray[s] < MinMesswertArray) 
      {MinMesswertArray = MesswertArray[s];}
  }

 // Switching relays

if (MaxMesswertArray >= VHigh)                     // Emergeny shut down of load and charge relays
   {digitalWrite (ChargeRelay, LOW);               // ChargeRelay OFF
    digitalWrite (LoadRelay, LOW);                 // LoadRelay OFF
    ChargeRelayStatus = ("Cx");                    // !!!!!!! This emergency shut down might require manual switch on of Load Relay, if voltage will not recover < VMaxOFF !!!!!!!
    LoadRelayStatus = ("Lx");}                     // !!!!!!! This emergency shut down might require manual switch on of Load Relay, if voltage will not recover < VMaxOFF !!!!!!!
 else if (MaxMesswertArray >= VMaxOFF)
   {digitalWrite (ChargeRelay, LOW);               // ChargeRelay OFF
    ChargeRelayStatus = ("Cx");}
 else if (MaxMesswertArray < VMaxOFF)              // Reversal of emergency shut down
   {digitalWrite (LoadRelay, HIGH);
    LoadRelayStatus = ("L+");}               
 else if (MaxMesswertArray <= VChaON)
     {digitalWrite (ChargeRelay, HIGH);            // ChargeRelay ON
      ChargeRelayStatus = ("C+");}

if (MaxMesswertArray >= VBalON)
    {digitalWrite (BalanceRelay, HIGH);            // BalanceRelay ON
     BalanceRelayStatus = ("B+");}
 else if (MaxMesswertArray <= VBalOFF)
     {digitalWrite (BalanceRelay, LOW);            // BalanceRelay OFF
      BalanceRelayStatus = ("Bx");}

if (MinMesswertArray <= VLow)                      // Emergency shut down of Charge and Load Relay as this threshold should not be met and indicates some error
   {digitalWrite (LoadRelay, LOW);                 // LoadRelay OFF
    digitalWrite (ChargeRelay, LOW);               // ChargeRelay OFF
    LoadRelayStatus = ("Lx");                      // !!!!!!! This might require manual charging, if voltage will not recover > VMinOFF !!!!!!!
    ChargeRelayStatus = ("Cx");}                   // !!!!!!! This might require manual charging, if voltage will not recover > VMinOFF !!!!!!!
 else if (MinMesswertArray <= VMinOFF)
   {digitalWrite (LoadRelay, LOW);                 // LoadRelay OFF, da auf NO angeschlossen
    LoadRelayStatus = ("Lx");}
 else if (MaxMesswertArray > VMinOFF)              // Reversal of emergency shut down
   {digitalWrite (ChargeRelay, HIGH);
    ChargeRelayStatus = ("C+");}               
 else if (MinMesswertArray >= VMinON)
     {digitalWrite (LoadRelay, HIGH);              // LoadRelay ON    
      LoadRelayStatus = ("L+");}


// Conversion of values to string displaying on OLED --> will return a rounded(!) string cut off after second decimal due to character limitation set in string definition
   voltage_cell_1_string = check_voltage_cell_1;
   voltage_cell_2_string = check_voltage_cell_2;
   voltage_cell_3_string = check_voltage_cell_3;
   voltage_cell_4_string = check_voltage_cell_4;
   voltage_battery_string = voltage_battery;

// Display on OLED
// Sequence MCP is 4-1-2-3
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvR14_tr);                                 // reduced ASCII 32 to 127
    u8g2.drawStr(0, 13, "1");                                           // cell number
    u8g2.drawStr(23, 13, voltage_cell_4_string.c_str());
    u8g2.drawStr(80, 13, voltage_battery_string.c_str());
    u8g2.drawStr(0, 30, "2");
    u8g2.drawStr(23, 30, voltage_cell_1_string.c_str());
    u8g2.drawStr(70, 34,LoadRelayStatus.c_str());
    u8g2.drawStr(0, 47, "3");
    u8g2.drawStr(23, 47, voltage_cell_2_string.c_str());
    u8g2.drawStr(100, 47, BalanceRelayStatus.c_str());
    u8g2.drawStr(0, 64, "4");
    u8g2.drawStr(23, 64, voltage_cell_3_string.c_str());
    u8g2.drawStr(70, 59, ChargeRelayStatus.c_str());
  } while ( u8g2.nextPage() );


   // Print results to Serial Monitor to 3 decimal places
   // Sequence 4-1-2-3
  Serial.print("#1 ");
  Serial.print(check_voltage_cell_4, 4);
  Serial.print("    #2 ");
  Serial.print(check_voltage_cell_1, 4);
  Serial.print("    #3 ");
  Serial.print(check_voltage_cell_2, 4);
  Serial.print("    #4 ");
  Serial.print(check_voltage_cell_3, 4);
  Serial.print("    MAX ");
  Serial.print(MaxMesswertArray);
  Serial.print("    MIN ");
  Serial.print(MinMesswertArray);
  Serial.print("    ");
  Serial.print(LoadRelayStatus);
  Serial.print("    ");
  Serial.print(ChargeRelayStatus);
  Serial.print("    ");
  Serial.print(BalanceRelayStatus);
  Serial.print("    ");
  Serial.print(voltage_cell_4_string);
  Serial.print("    ");
  Serial.print(voltage_cell_1_string);
  Serial.print("    ");
  Serial.print(voltage_cell_2_string);
  Serial.print("    ");
  Serial.print(voltage_cell_3_string);
  Serial.print("    ");
  Serial.println(voltage_battery_string);

  // Reset temporary storage of voltages per cell

      check_voltage_cell_1 = 0.000;
      check_voltage_cell_2 = 0.000;
      check_voltage_cell_3 = 0.000;
      check_voltage_cell_4 = 0.000;

}
