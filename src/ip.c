#include "net.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 */
void ip_in(buf_t *buf, uint8_t *src_mac)
{
    // TO-DO
    if(buf->len >= 20){
        ip_hdr_receive_t *hdr = buf->data;
        size_t hdr_len = hdr->version_and_hdr_len & 0x0f;
        uint16_t total_len = swap16(hdr->total_len16);
        if((hdr -> version_and_hdr_len & 0xf0) >> 4 == IP_VERSION_4 && total_len <= buf->len){
            uint16_t checksum16_sender = hdr->hdr_checksum16;
            hdr->hdr_checksum16 = 0;
            uint16_t checksum_receiver = checksum16(hdr,hdr_len*4);
            if(checksum_receiver == checksum16_sender){
                hdr->hdr_checksum16 = checksum16_sender;
                if(memcmp(hdr->dst_ip,net_if_ip,4) == 0){
                    if(total_len < buf->len){
                        buf_remove_padding(buf,buf->len - total_len);
                    }
                    switch (hdr->protocol){
                        case NET_PROTOCOL_UDP:{
                            buf_remove_header(buf,hdr_len*4);
                            net_in(buf,hdr->protocol,hdr->src_ip);
                            break;
                        }
                        case NET_PROTOCOL_ICMP:{
                            buf_remove_header(buf,hdr_len*4);
                            net_in(buf,hdr->protocol,hdr->src_ip);
                            break;
                        }
                        default:{
                            icmp_unreachable(buf,hdr->src_ip,ICMP_CODE_PROTOCOL_UNREACH);
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief 处理一个要发送的ip分片
 * 
 * @param buf 要发送的分片
 * @param ip 目标ip地址
 * @param protocol 上层协议
 * @param id 数据包id
 * @param offset 分片offset，必须被8整除
 * @param mf 分片mf标志，是否有下一个分片
 */
void ip_fragment_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol, int id, uint16_t offset, int mf)
{
    // TO-DO
    size_t buf_len = buf->len;
    buf_add_header(buf,sizeof(ip_hdr_t));
    ip_hdr_t *hdr = (ip_hdr_t*)buf->data;
    hdr->version = IP_VERSION_4;
    hdr->hdr_len = 5;
    hdr->tos = 0;
    hdr->total_len16 = swap16(buf->len);
    hdr->id16 = swap16(id);
    if(mf > 0){
        hdr->flags_fragment16 = swap16(IP_MORE_FRAGMENT + offset);
    }
    else{
        hdr->flags_fragment16 = swap16(offset);
    }
    hdr->ttl = 64;
    hdr->protocol = protocol;
    hdr->hdr_checksum16 = 0;
    memcpy(hdr->src_ip,net_if_ip,4);
    memcpy(hdr->dst_ip,ip,4); 
    hdr->hdr_checksum16 = checksum16(hdr,hdr->hdr_len*4);
    arp_out(buf,ip);
}

/**
 * @brief 处理一个要发送的ip数据包
 * 
 * @param buf 要处理的包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void ip_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol)
{
    // TO-DO
    static int id = 0;
    uint16_t offset = 0;
    size_t len = buf->len;
    while(len > ETHERNET_MAX_TRANSPORT_UNIT - 20){
        size_t buf_len = (ETHERNET_MAX_TRANSPORT_UNIT - 20)/8 * 8;
        buf_init(&txbuf,buf_len);
        txbuf.len = buf_len;
        memcpy(txbuf.data,buf->data+offset*8,buf_len);
        ip_fragment_out(&txbuf,ip,protocol,id,offset,1);
        offset += buf_len/8;
        len-=buf_len;
    }
    buf_init(&txbuf,len);
    txbuf.len = len;
    memcpy(txbuf.data,buf->data+offset*8,len);
    ip_fragment_out(&txbuf,ip,protocol,id++,offset,0);
}

/**
 * @brief 初始化ip协议
 * 
 */
void ip_init()
{
    net_add_protocol(NET_PROTOCOL_IP, ip_in);
}