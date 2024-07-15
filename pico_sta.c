#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"

#define BUTTON_PIN 15 //Button is connected to GPIO pin 15, which is physical pin 20

volatile bool button_pressed = false; //button pressed bool is false by default. Volatile because it can change at any moment
static const int port = WIFI_PORT; //WIFI_PORT is defined in CMakeLists.txt as 8080

/*
* Function called when interrupt is triggered (Interrupt Service Routine)
*/
void gpio_callback(uint gpio, uint32_t events){
    button_pressed = true;
}

/*
* sendMsg function sends an acknowledge message to the station pico.  
*/
void sendMsg(const ip_addr_t* remote_address){
    button_pressed = false; //sendMsg is called when button is pressed. It is then made false so it can be pressed again later.
    const char *message = "Turn On"; //Message to be sent
    int msg_length = strlen(message) + 1; //Message length is the length of the string plus one
    
    struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, msg_length, PBUF_RAM); //Memory for the pbuf struct is allocated.
    
    memcpy(p->payload, message, msg_length); //The message is copied in the payload of the pbuf struct

    struct udp_pcb* pcb = udp_new(); //udp pcb block created
    udp_bind(pcb, IP_ADDR_ANY, port); //udp pcb is binded to any available IP address and to port WIFI_PORT (8080 in CMakeLists.txt)
    udp_connect(pcb, remote_address, port); //udp pcb block is connected to the remote address of the station
    int err = udp_send(pcb, p); //message is sent, udp_send returns lwIP error code
    if(err != ERR_OK){
        printf("\nmessage could not be sent, error %d.", err); //print error message in case error appears
    }
    pbuf_free(p); //pbuf is dereferenced
    udp_remove(pcb); //udp pcb block is removed
}

/*
* udp receive function prototype. Function must have this exact form to be used with udp_recv
*/
static void udp_recv_function(void *arg, struct udp_pcb *recv_pcb, struct pbuf *p, const ip_addr_t *source_addr, u16_t source_port){
    (void)arg; //Casted as void since they are not used in this function. Not casting might result in warnings
    (void)recv_pcb;
    (void)source_port;
    (void)source_addr;

    char received_msg[p->tot_len]; //Character array is created with size of the total length of message p
    memcpy(received_msg, p->payload, p->tot_len); //Payload of the message is copied to the character array

    /*
    * Access point expects to receive the message "Ack". If the message received is "Ack" 
    * the LED is turned on for half a second 
    */
    if (strcmp(received_msg, "Ack") == 0){
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        sleep_ms(100);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
    }
    pbuf_free(p); //pbuf is dereferenced
}

int main(){
    stdio_init_all(); //Initialise all standard I/O interfaces  

    gpio_init(BUTTON_PIN); //Button pin (GPIO 15) is initialised
    gpio_set_dir(BUTTON_PIN, GPIO_IN); //Button pin is set as input
    gpio_pull_up(BUTTON_PIN); //Defult value of button pin is 1. Press of the button grounds the pin and is 0. This triggers the ISR

    /*
    * Set up the GPIO interrupt on the button pin to trigger on the falling edge.
    * Press of the button triggers the interrupt.
    */
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    ip4_addr_t remote_address; //Address of the access point is defined here
    IP4_ADDR(&remote_address, 192, 168, 4, 1); //Address is 192.168.4.1

    /*
    * cyw43 wireless chip is initialised with country. Change country accordingly
    */
    if(cyw43_arch_init_with_country(CYW43_COUNTRY_GREECE)) {
        printf("\nfailed to initialise"); //If chip could not be initialised error message is printed
        return 1; //error code is returned
    }

    cyw43_arch_enable_sta_mode(); //Station mode is enabled

    const char *ap_name = WIFI_SSID; //WIFI_SSID is defined in CMakeLists.txt
    const char *password = PASSWORD; //PASSWORD is defined in CMakeLists.txt
    /*
    * Station attempts to connect to the access point. If it can't connect after 30 seconds
    * it times out, prints an error message and returns an error value. 
    */
    if(cyw43_arch_wifi_connect_timeout_ms(ap_name, password, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0){
        printf("\nfailed to connect to access point"); //error message
        return 1; //error value
    }

    udp_init(); //udp module is initialised
    struct udp_pcb *udp = udp_new(); //new udp pcb block is created
    if(!udp){
        printf("\nfailed to create udp"); //error message is printed if udp pcb could not be created
    }
    if (ERR_OK != udp_bind(udp, IP_ADDR_ANY, port)){
        printf("\nfailed to bind to port %u", port); //error message is printed if udp pcb could not be binded to WIFI_PORT
    }

    udp_recv(udp, udp_recv_function, (void *)NULL); //Receive callback is set for the udp pcb. Callback function is the one in line 48

    while(true){
        //button_pressed is made true by the interrupt service routine
        if(button_pressed == true){
            sleep_ms(250); //debounce time
            sendMsg(&remote_address); //message is sent to the access point
        }
        sleep_ms(100); //sleep for 100 ms
        cyw43_arch_poll();//Poll the cyw43 architecture to process any pending events. Must be called regularly
    }

    return 0;
}