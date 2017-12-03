#include "mgos_dallas_rmt.h"

  
  struct ow_struct {
   int one_wire_count;        // Number of sensors found on the 1-Wire bus
   char one_wire_addr[10][8];  // Sensors addresses
   float sensor_temp[10];
   DallasRmt* obj;
   int ch;
   int pin;
   int rmt_rx;
   int rmt_tx;
   char name[10];
};  
extern struct ow_struct one_wire_ch1;
extern struct ow_struct one_wire_ch2;
extern struct ow_struct one_wire_ch3;
extern int triac_set_value[];