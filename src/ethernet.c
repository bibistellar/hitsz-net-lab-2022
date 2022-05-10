#include "ethernet.h"
#include "utils.h"
#include "driver.h"
#include "arp.h"
#include "ip.h"

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 */
void ethernet_in(buf_t *buf)
{
    // TO-DO
    if(buf->len>14){
        ether_hdr_t *hdr = (ether_hdr_t*)buf->data;
        uint16_t protocol;
        uint8_t src[6],dst[6];
        protocol = hdr->protocol16;
        protocol = swap16(protocol);
        memcpy(dst,hdr->dst,6);
        memcpy(src,hdr->src,6);
        buf_remove_header(buf,14);
        if(net_in(buf,protocol,src) != 0){
            printf("failed!\n");
        }
    }
}
/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的数据包
 * @param mac 目标MAC地址
 * @param protocol 上层协议
 */
void ethernet_out(buf_t *buf, const uint8_t *mac, net_protocol_t protocol)
{
    // TO-DO
    uint16_t protocol_reverse = swap16(protocol);
    if(buf->len<46){
        buf_add_padding(buf,46 - buf->len);
    }

    buf_add_header(buf,sizeof(ether_hdr_t));
    ether_hdr_t *hdr = (ether_hdr_t*)buf->data;
    memcpy(hdr,mac,6);
    memcpy(hdr->src,net_if_mac,6);
    hdr->protocol16 = protocol_reverse;
    driver_send(buf);
}
/**
 * @brief 初始化以太网协议
 * 
 */
void ethernet_init()
{
    buf_init(&rxbuf, ETHERNET_MAX_TRANSPORT_UNIT + sizeof(ether_hdr_t));
}

/**
 * @brief 一次以太网轮询
 * 
 */
void ethernet_poll()
{
    if (driver_recv(&rxbuf) > 0){
        ethernet_in(&rxbuf);
    }
}