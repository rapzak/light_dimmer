/*
author: Kasper Jepsen @ Rapzak
description: 4 Ch light dimmer / Fan control with mqtt (50hz)
version: 1.0
*/

#include "mgos.h"
#include "mgos_gpio.h"
#include "esp32_hw_timers.h" // needed?
#include "mgos_pwm.h"
#include "mqtt.h"
#include "main.h"


 //Step time in us - The last 2 values are tuned to avoid overrun. This liniarizes the sinus so step size is quite equal
int  lin_tbl[] = {1416,593,464,400,362,337,320,309,302,299,299,302,309,320,337,362,400,464,593,800,50}; 

// Set points 0-20. 0=Off - 20 is full on.
 int triac_set_value[] = {20,20,20,20};
// Config for pin (ISR function can not run get_cfg)
 int  ch1_pin;
 int  ch2_pin;
 int  ch3_pin;
 int  ch4_pin;

//Counter for the actual step
 signed int  counter = -1;
//Timer ref
 mgos_timer_id  tim_ref;

struct ow_struct one_wire_ch1;
struct ow_struct one_wire_ch2;
struct ow_struct one_wire_ch3;


void setup_onewire(struct ow_struct *one_wire_chx) {
  one_wire_chx->obj = mgos_dallas_rmt_create(mgos_onewire_rmt_create(one_wire_chx->pin,one_wire_chx->rmt_rx,one_wire_chx->rmt_tx));
  mgos_dallas_rmt_set_wait_for_conversion(one_wire_chx->obj,true);
  mgos_dallas_rmt_begin(one_wire_chx->obj);
  if ((one_wire_chx->one_wire_count = mgos_dallas_rmt_get_device_count(one_wire_chx->obj)) == 0) 
    return;


  for (int i = 0; i < one_wire_chx->one_wire_count; i++) {
     mgos_dallas_rmt_get_address(one_wire_chx->obj, one_wire_chx->one_wire_addr[i],i);
}
}

void loop_onewire(void *arg) {
  struct ow_struct* one_wire_chx =(struct ow_struct*) arg;
  mgos_dallas_rmt_request_temperatures(one_wire_chx->obj);
  for (int i = 0; i < one_wire_chx->one_wire_count; i++) {
	one_wire_chx->sensor_temp[i]  = mgos_dallas_rmt_get_tempc(one_wire_chx->obj, one_wire_chx->one_wire_addr[i])/100.0;
    //printf("--------Sens#%d temperature: %f *C -- ch_%d -- pin %d\n", i + 1, one_wire_chx->sensor_temp[i],one_wire_chx->ch,one_wire_chx->pin );
  }
  
}

static void start_onewire( void *arg) {
	struct ow_struct* one_wire_chx =(struct ow_struct*) arg;
	setup_onewire(one_wire_chx); //mgos_sys_config_get_one_wire_ch1
	mgos_set_timer(10000, 1, loop_onewire,arg); // 3000
}

 

void IRAM trigger(void *arg) {
	(void) arg;
	if (triac_set_value[0]==20 || triac_set_value[0]<counter)
		mgos_gpio_write(ch1_pin, 0);
	else
		if (triac_set_value[0]==counter)
			mgos_gpio_write(ch1_pin, 1);
	
	if (triac_set_value[1]==20 || triac_set_value[1]<counter)
		mgos_gpio_write(ch2_pin, 0);
	else
		if (triac_set_value[1]==counter)
			mgos_gpio_write(ch2_pin, 1);
		
	if (triac_set_value[2]==20 || triac_set_value[2]<counter)
		mgos_gpio_write(ch3_pin, 0);
	else
		if (triac_set_value[2]==counter)
			mgos_gpio_write(ch3_pin, 1);
		
	if (triac_set_value[3]==20 || triac_set_value[3]<counter)
		mgos_gpio_write(ch4_pin, 0);
	else
		if (triac_set_value[3]==counter)
			mgos_gpio_write(ch4_pin, 1);

	if (counter > 20){
		counter=-1;
		mgos_clear_timer(tim_ref);
		} 
	else{
		mgos_clear_timer(tim_ref);
		tim_ref = mgos_set_hw_timer(lin_tbl[counter], 0, trigger,NULL); 
		counter = counter +1;
		}
}


//This function starts the 20 steps timer sequence for each zerocross of the line signal
static void IRAM int_ext(int pin, void *arg) {
  (void) pin;
  (void) arg;
  if (counter == -1){ // good for not start time by error / bad can stop everything
	counter = 0;
	tim_ref = mgos_set_hw_timer(10, 0, trigger,NULL);//MGOS_ESP32_HW_TIMER_NMI
  }
}


enum  mgos_app_init_result mgos_app_init(void) {

	// get the pin numbers from config
	ch1_pin = mgos_sys_config_get_triac_ch1_out();
	ch2_pin = mgos_sys_config_get_triac_ch2_out();
	ch3_pin = mgos_sys_config_get_triac_ch3_out();
	ch4_pin = mgos_sys_config_get_triac_ch4_out();
	// Setup the GPIO pins
	mgos_gpio_set_mode(ch1_pin,MGOS_GPIO_MODE_OUTPUT);
	mgos_gpio_set_mode(ch2_pin,MGOS_GPIO_MODE_OUTPUT);
	mgos_gpio_set_mode(ch3_pin,MGOS_GPIO_MODE_OUTPUT);
	mgos_gpio_set_mode(ch4_pin,MGOS_GPIO_MODE_OUTPUT);
	mgos_gpio_set_mode(15,MGOS_GPIO_MODE_OUTPUT);
	//Setup input pin and interupt
	mgos_gpio_set_mode(mgos_sys_config_get_triac_zc_in(),MGOS_GPIO_MODE_INPUT);
	mgos_gpio_set_int_handler_isr(mgos_sys_config_get_triac_zc_in(), MGOS_GPIO_INT_EDGE_POS, int_ext, NULL);
	mgos_gpio_enable_int(mgos_sys_config_get_triac_zc_in());
	
	
	//setup one wire
	one_wire_ch1.ch = 1;
	one_wire_ch2.ch = 2;
	one_wire_ch3.ch = 3;
	one_wire_ch1.pin = 26;
	one_wire_ch2.pin = 27;
	one_wire_ch3.pin = 14;
	
	one_wire_ch1.rmt_tx = 1;
	one_wire_ch2.rmt_tx = 3;
	one_wire_ch3.rmt_tx = 5;
	one_wire_ch1.rmt_rx = 2;
	one_wire_ch2.rmt_rx = 4;
	one_wire_ch3.rmt_rx = 6;
	
	strcpy(one_wire_ch1.name, "wire1");
	strcpy(one_wire_ch2.name, "wire2");
	strcpy(one_wire_ch3.name, "wire3");
	
	
	//start onewire timer
	mgos_set_timer(5000, 0, start_onewire,&one_wire_ch1);
	mgos_set_timer(6000, 0, start_onewire,&one_wire_ch2);
	mgos_set_timer(7000, 0, start_onewire,&one_wire_ch3);
	init_mqtt(); // true
    return MGOS_APP_INIT_SUCCESS;
}














