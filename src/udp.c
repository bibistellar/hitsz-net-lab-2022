#include "udp.h"
#include "ip.h"
#include "icmp.h"

/**
 * @brief udp处理程序表
 * 
 */
map_t udp_table;

/**
 * @brief udp伪校验和计算
 * 
 * @param buf 要计算的包
 * @param src_ip 源ip地址
 * @param dst_ip 目的ip地址
 * @return uint16_t 伪校验和
 */
static uint16_t udp_checksum(buf_t *buf, uint8_t *src_ip, uint8_t *dst_ip)
{
    // TO-DO
    uint16_t length = buf->len; 
    int if_pad = 0;
    if(buf->len % 2 != 0){
        buf_add_padding(buf,1);
        if_pad = 1;
    }
    buf_add_header(buf,sizeof(udp_peso_hdr_t));
    udp_peso_hdr_t * hdr = buf->data;
    hdr->placeholder = 0;
    hdr->protocol = NET_PROTOCOL_UDP;
    hdr->total_len16 = swap16(length);
    memcpy(hdr->src_ip,src_ip,4);
    memcpy(hdr->dst_ip,dst_ip,4);
    uint16_t result =  checksum16(buf->data,buf->len);
    buf_remove_header(buf,sizeof(udp_peso_hdr_t));
    if(if_pad){
        buf_remove_padding(buf,1);
    }
    return result;
}

/**
 * @brief 处理一个收到的udp数据包
 * 
 * @param buf 要处理的包
 * @param src_ip 源ip地址
 */
void udp_in(buf_t *buf, uint8_t *src_ip)
{
    // TO-DO
    printf("udp_in!\n");
    if(buf->len >= sizeof(udp_hdr_t)){
        uint8_t src_ip_temp[4];
        udp_hdr_t *hdr = buf->data;
        memcpy(src_ip_temp,src_ip,4);
        uint16_t src_checksum = hdr->checksum16;
        hdr->checksum16 = 0;
        hdr->checksum16 = udp_checksum(buf,src_ip,net_if_ip);
        if(hdr->checksum16 == src_checksum){
            uint16_t port = swap16(hdr->dst_port16);            
            udp_handler_t* handler = (udp_handler_t*)map_get(&udp_table,&port);
            if(handler){
                buf_remove_header(buf,sizeof(udp_hdr_t));
                ip_print(src_ip_temp);
                (*handler)(buf->data,buf->len,src_ip_temp,swap16(hdr->src_port16));
            }
            else{
                buf_add_header(buf,sizeof(ip_hdr_t));
                ip_hdr_t *ip_hdr = buf;
                memcpy(ip_hdr -> dst_ip,net_if_ip,4);
                ip_hdr -> hdr_len = 5;
                icmp_unreachable(buf,src_ip_temp,ICMP_CODE_PORT_UNREACH);
            }
        }
    }
}

/**
 * @brief 处理一个要发送的数据包
 * 
 * @param buf 要处理的包
 * @param src_port 源端口号
 * @param dst_ip 目的ip地址
 * @param dst_port 目的端口号
 */
void udp_out(buf_t *buf, uint16_t src_port, uint8_t *dst_ip, uint16_t dst_port)
{
    // TO-DO
    buf_add_header(buf,sizeof(udp_hdr_t));
    udp_hdr_t *hdr = buf->data;
    hdr->dst_port16 = swap16(dst_port);
    hdr->src_port16 = swap16(src_port);
    hdr->total_len16 = swap16(buf->len);
    hdr->checksum16 = 0;
    hdr->checksum16 = udp_checksum(buf,net_if_ip,dst_ip);
    ip_out(buf,dst_ip,NET_PROTOCOL_UDP);
}

/**
 * @brief 初始化udp协议
 * 
 */
void udp_init()
{
    map_init(&udp_table, sizeof(uint16_t), sizeof(udp_handler_t), 0, 0, NULL);
    net_add_protocol(NET_PROTOCOL_UDP, udp_in);
}

/**
 * @brief 打开一个udp端口并注册处理程序
 * 
 * @param port 端口号
 * @param handler 处理程序
 * @return int 成功为0，失败为-1
 */
int udp_open(uint16_t port, udp_handler_t handler)
{
    map_set(&udp_table, &port, &handler);
    udp_handler_t *p = (udp_handler_t*)map_get(&udp_table,&port);
    return map_set(&udp_table, &port, &handler);
}

/**
 * @brief 关闭一个udp端口
 * 
 * @param port 端口号
 */
void udp_close(uint16_t port)
{
    map_delete(&udp_table, &port);
}

/**
 * @brief 发送一个udp包
 * 
 * @param data 要发送的数据
 * @param len 数据长度
 * @param src_port 源端口号
 * @param dst_ip 目的ip地址
 * @param dst_port 目的端口号
 */
void udp_send(uint8_t *data, uint16_t len, uint16_t src_port, uint8_t *dst_ip, uint16_t dst_port)
{
    ip_print(dst_ip);
    buf_init(&txbuf, len);
    memcpy(txbuf.data, data, len);
    udp_out(&txbuf, src_port, dst_ip, dst_port);
}