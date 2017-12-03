#include "frozen/frozen.h"
#include "mgos_mqtt.h"
#include "mgos.h"
#include "mgos_gpio.h"
#include "mgos_pwm.h"
#include "main.h"

// Set function for CHx 0-1 in value // DOES SSR run with pwm??
void set_relay_ch(int pin,int value){
	if (value ==1)
		mgos_pwm_set(pin, 500, 1 );	
	else
		mgos_pwm_set(pin, 500, 0 );		
}

// Set function for CHx 0-100% in value
void set_pwm_ch(int pin,int value){
	mgos_pwm_set(pin, 1000, value/100.0 );
}



static void relay_handler(struct mg_connection *c, const char *topic, int topic_len,const char *msg, int msg_len, void *userdata) {				
	int ch, value;
    json_scanf(msg, msg_len, "{ch:%d,value:%d}",&ch,&value);		
	switch(ch){
		case 1 : set_relay_ch(mgos_sys_config_get_relay_ch1_out(),value);
		 break;
		case 2 : set_relay_ch(mgos_sys_config_get_relay_ch2_out(),value);
		 break;
		case 3 : set_relay_ch(mgos_sys_config_get_relay_ch3_out(),value);
		 break;
		case 4 : set_relay_ch(mgos_sys_config_get_relay_ch4_out(),value);
		 break;
	}
	LOG(LL_ERROR, ("Got message on topic %.*s - ch:%d - Value:%d\n", topic_len, topic,ch,value));
}

static void pwm_handler(struct mg_connection *c, const char *topic, int topic_len,const char *msg, int msg_len, void *userdata) {				
	int ch, value;
    json_scanf(msg, msg_len, "{ch:%d,value:%d}",&ch,&value);		
	switch(ch){
		case 1 : set_pwm_ch(mgos_sys_config_get_pwm_ch1_out(),value);
		 break;
		case 2 : set_pwm_ch(mgos_sys_config_get_pwm_ch2_out(),value);
		 break;
		case 3 : set_pwm_ch(mgos_sys_config_get_pwm_ch3_out(),value);
		 break;
		case 4 : set_pwm_ch(mgos_sys_config_get_pwm_ch4_out(),value);
		 break;
	}
	LOG(LL_ERROR, ("Got message on topic %.*s - ch:%d - Value:%d\n", topic_len, topic,ch,value));
}

static void triac_handler(struct mg_connection *c, const char *topic, int topic_len,const char *msg, int msg_len, void *userdata) {				
	int ch, value;
    json_scanf(msg, msg_len, "{ch:%d,value:%d}",&ch,&value);

	if (value>=0 && value<=20)
		value = 20-value;   // Set Value
	else
		value = 20;	
	switch(ch){
		case 1 : triac_set_value[0]=value;
		 break;
		case 2 : triac_set_value[1]=value;
		 break;
		case 3 : triac_set_value[2]=value;
		 break;
		case 4 : triac_set_value[3]=value;
		 break;
	}
	LOG(LL_ERROR, ("Got message on topic %.*s - ch:%d - Value:%d\n", topic_len, topic,ch,value));
}


int print_my_struct(struct json_out *out, va_list *ap) {
  struct ow_struct *p = va_arg(*ap, struct ow_struct *);
  int len=0;
  char str[17];
	for( int i = 0; i < p->one_wire_count; i++){
		if (i>0) len += json_printf(out, ",");
		snprintf(str,sizeof(str),"%02x%02x%02x%02x%02x%02x%02x%02x", p->one_wire_addr[i][0],p->one_wire_addr[i][1],p->one_wire_addr[i][2],p->one_wire_addr[i][3],p->one_wire_addr[i][4],p->one_wire_addr[i][5],p->one_wire_addr[i][6],p->one_wire_addr[i][7]);   
		len += json_printf(out, "%Q : %.2f", str, p->sensor_temp[i]);
	}
  return len;
}


static void mqtt_publish( void *arg) {
	struct ow_struct* one_wire_chx =(struct ow_struct*) arg;
	struct mbuf fb;
    struct json_out out = JSON_OUT_MBUF(&fb);
    mbuf_init(&fb, 500);
    json_printf(&out, "{%M}", print_my_struct, one_wire_chx); 
	mgos_mqtt_pub(one_wire_chx->name, fb.buf, fb.len, 1, 0);   /* Publish */
	mbuf_free(&fb);
}



bool init_mqtt(void){
	mgos_mqtt_init();
	mgos_mqtt_sub(mgos_sys_config_get_relay_mqtt_string(), relay_handler, NULL);
	mgos_mqtt_sub(mgos_sys_config_get_pwm_mqtt_string(), pwm_handler, NULL);	
	mgos_mqtt_sub(mgos_sys_config_get_triac_mqtt_string(), triac_handler, NULL);
	
	
	mgos_set_timer(10000, 1, mqtt_publish,&one_wire_ch1);
	mgos_set_timer(10000, 1, mqtt_publish,&one_wire_ch2);
	mgos_set_timer(10000, 1, mqtt_publish,&one_wire_ch3);
	
	
	return true;
}
