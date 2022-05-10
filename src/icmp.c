#include "net.h"
#include "icmp.h"
#include "ip.h"

/**
 * @brief 发送icmp响应
 * 
 * @param req_buf 收到的icmp请求包
 * @param src_ip 源ip地址
 */
static void icmp_resp(buf_t *req_buf, uint8_t *src_ip)
{
    // TO-DO
    //buf_init(&txbuf,req_buf->len);
    icmp_hdr_t *hdr = req_buf->data;
    hdr->type = ICMP_TYPE_ECHO_REPLY;
    hdr->code = 0;
    hdr->checksum16 = 0;
    hdr->checksum16 = checksum16(hdr,req_buf->len);
    ip_out(req_buf,src_ip,NET_PROTOCOL_ICMP);
}

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_ip 源ip地址
 */
void icmp_in(buf_t *buf, uint8_t *src_ip)
{
    // TO-DO
    if(buf->len >= 8){
        icmp_hdr_t *hdr = buf->data;
        if(hdr->type == ICMP_TYPE_ECHO_REQUEST){
            icmp_resp(buf,src_ip);
        }
    }
}

/**
 * @brief 发送icmp不可达
 * 
 * @param recv_buf 收到的ip数据包
 * @param src_ip 源ip地址
 * @param code icmp code，协议不可达或端口不可达
(buf_t *recv_buf, uint8_t *src_ip, icmp_code_t code) */
void icmp_unreachable(buf_t *recv_buf, uint8_t *src_ip, icmp_code_t code)
{
    ip_hdr_t *err_ip_hdr = recv_buf->data;
    buf_init(&txbuf,err_ip_hdr->hdr_len*4+8);
    memcpy(txbuf.data,recv_buf->data,txbuf.len);
    buf_add_header(&txbuf,8);
    icmp_hdr_t *hdr = txbuf.data;
    hdr->type = ICMP_TYPE_UNREACH;
    hdr->code = code;
    hdr->checksum16 = 0;
    hdr->id16 = 0;
    hdr->seq16 = 0;
    hdr->checksum16 = checksum16(hdr,txbuf.len);
    ip_out(&txbuf,src_ip,NET_PROTOCOL_ICMP);
}

/**
 * @brief 初始化icmp协议
 * 
 */
void icmp_init(){
    net_add_protocol(NET_PROTOCOL_ICMP, icmp_in);
}