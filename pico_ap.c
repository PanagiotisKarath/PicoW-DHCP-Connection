#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "dhcpserver.h"

#define LED_PIN 15 //LED is connected to GPIO pin 15, which is physical pin 20

static const int port = WIFI_PORT; //WIFI_PORT is defined in CMakeLists.txt as 8080

/*
* sendMsg function sends an acknowledge message to the station pico.  
*/
void sendMsg(const ip_addr_t* remote_address){
    const char *ack_msg = "Ack"; //Message to be sent
    int msg_length = strlen(ack_msg) + 1; //Message length is the length of the string plus one
    
    struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, msg_length, PBUF_RAM); //Memory for the pbuf struct is allocated.
    
    memcpy(p->payload, ack_msg, msg_length); //The message is copied in the payload of the pbuf struct

    struct udp_pcb* pcb = udp_new(); //udp pcb block created
    udp_bind(pcb, IP_ADDR_ANY, port); //udp pcb is binded to any available IP address and to port WIFI_PORT (8080 in CMakeLists.txt)
    udp_connect(pcb, remote_address, port); //udp pcb block is connected to the remote address of the station
    int err = udp_send(pcb, p); //message is sent, udp_send returns lwIP error code
    if(err != ERR_OK){
        printf("\nacknowledge message could not be sent, error %d.", err); //print error message in case error appears
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

    char received_msg[p->tot_len]; //Character array is created with size of the total length of message p
    memcpy(received_msg, p->payload, p->tot_len); //Payload of the message is copied to the character array

    /*
    * Access point expects to receive the message "Turn On". If the message received is "Turn On" 
    * the LED is turned on for half a second 
    */
    if (strcmp(received_msg, "Turn On") == 0){
        gpio_put(LED_PIN, true);
        sleep_ms(500);
        gpio_put(LED_PIN, false);
    }
    /*
    * Acknowledge message is sent even if the message received was wrong.
    * I used the acknowledge message just to let the station know that the access point
    * received a message, whether that be the correct one or not. 
    */
    sendMsg(source_addr);
    pbuf_free(p); //pbuf is dereferenced
}

int main(){
    stdio_init_all(); //Initialise all standard I/O interfaces    

    gpio_init(LED_PIN); //LED pin (GPIO 15) is initialised
    gpio_set_dir(LED_PIN, GPIO_OUT); //LED pin is set as output

    /*
    * cyw43 wireless chip is initialised with country. Change country accordingly
    */
    if(cyw43_arch_init_with_country(CYW43_COUNTRY_GREECE)) {
        printf("\nfailed to initialise"); //If chip could not be initialised error message is printed
        return 1; //error code is returned
    }

    const char *ap_name = WIFI_SSID; //WIFI_SSID is defined in CMakeLists.txt
    const char *password = PASSWORD; //PASSWORD is defined in CMakeLists.txt
    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK); //Access Point mode is enabled
    
    ip4_addr_t gateway, netmask; //Gateway IP and subnet mask are defined here
    IP4_ADDR(&gateway, 192, 168, 4, 1); //Gateway IP is 192.168.4.1. This is basically the remote IP the station will send the message
    IP4_ADDR(&netmask, 255, 255, 255, 0); //Subnet mask is 255.255.255.0

    dhcp_server_t dhcp_server; //DHCP server is created
    dhcp_server_init(&dhcp_server, &gateway, &netmask); //DHCP server is initialised with the gateway IP and the subnet mask defined before

    udp_init(); //udp module is initialised
    struct udp_pcb *udp = udp_new(); //new udp pcb block is created
    if(!udp){
        printf("\nfailed to create udp"); //error message is printed if udp pcb could not be created
    }
    if (ERR_OK != udp_bind(udp, IP_ADDR_ANY, port)){
        printf("\nfailed to bind to port %u", port); //error message is printed if udp pcb could not be binded to WIFI_PORT
    }

    udp_recv(udp, udp_recv_function, (void *)NULL); //Receive callback is set for the udp pcb. Callback function is the one in line 39
    
    while(true) {
        cyw43_arch_poll(); //Poll the cyw43 architecture to process any pending events. Must be called regularly
        sleep_ms(1); //Sleep for 1 ms
    }

    return 0;
}