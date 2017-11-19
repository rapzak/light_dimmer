  
#include <mgos_arduino_onewire.h>
#include <mgos_arduino_dallas_temp.h>
  
  struct ow_struct {
   int one_wire_count;        // Number of sensors found on the 1-Wire bus
   char one_wire_addr[10][8];  // Sensors addresses
   float sensor_temp[10];
   DallasTemperature *obj;
   int ch;
   int pin;
   char name[10];
};  
extern struct ow_struct one_wire_ch1;
extern struct ow_struct one_wire_ch2;
extern struct ow_struct one_wire_ch3;
  
  
  extern int triac_set_value[];