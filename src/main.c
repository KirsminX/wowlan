/* 使用 C 语言实现的版本
 * 为 mt7621.Musl 设计，因为 Musl 架构使用 Rust 太难编译
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <unistd.h>
 #include <ctype.h>
 #include <errno.h>

 #define MAGIC_PACKET_SIZE 102
 #define BROADCAST_PORT 9

 // 错误类型枚举
 typedef enum {
     ERROR_NONE,
     ERROR_MAC,
     ERROR_SUBNET,
     ERROR_SOCKET,
     ERROR_IPV6_NOT_SUPPORTED  // 添加IPv6不支持的错误类型
 } ValidationError;

 // 解析 MAC 地址
 ValidationError parse_mac(const char *mac_str, unsigned char *mac) {
     int values[6];
     int count;
     char sep = '\0';

     // 尝试分隔符格式 (xx:xx:xx:xx:xx:xx 或 xx-xx-xx-xx-xx-xx)
     count = sscanf(mac_str, "%x%c%x%c%x%c%x%c%x%c%x",
                    &values[0], &sep, &values[1], &sep, &values[2],
                    &sep, &values[3], &sep, &values[4], &sep, &values[5]);

     // 检查分隔符是否一致
     if (count == 11 && (sep == ':' || sep == '-')) {
         for (int i = 0; i < 6; i++) {
             if (values[i] < 0 || values[i] > 255) return ERROR_MAC;
             mac[i] = (unsigned char)values[i];
         }
         return ERROR_NONE;
     }

     // 尝试无分隔符格式 (xxxxxxxxxxxx)
     if (strlen(mac_str) == 12) {
         for (int i = 0; i < 12; i++) {
             if (!isxdigit(mac_str[i])) return ERROR_MAC;
         }

         for (int i = 0; i < 6; i++) {
             char byte_str[3] = {mac_str[i*2], mac_str[i*2+1], '\0'};
             char *end;
             long val = strtol(byte_str, &end, 16);
             if (*end != '\0' || val < 0 || val > 255) return ERROR_MAC;
             mac[i] = (unsigned char)val;
         }
         return ERROR_NONE;
     }

     return ERROR_MAC;
 }

 // 计算广播地址
 ValidationError get_broadcast_addr(const char *subnet_str, struct sockaddr_in *broadcast_addr) {
     char ip_str[INET_ADDRSTRLEN];
     int prefix_len;

     // 解析 CIDR 格式
     if (sscanf(subnet_str, "%15[^/]/%d", ip_str, &prefix_len) != 2) {
         return ERROR_SUBNET;
     }

     // 验证前缀长度
     if (prefix_len < 0 || prefix_len > 32) {
         return ERROR_SUBNET;
     }

     // 转换 IP 地址
     struct in_addr ip_addr;
     if (inet_pton(AF_INET, ip_str, &ip_addr) != 1) {
         // 检查是否为IPv6地址
         struct in6_addr ip6_addr;
         if (inet_pton(AF_INET6, ip_str, &ip6_addr) == 1) {
             // 是IPv6地址，返回不支持错误
             return ERROR_IPV6_NOT_SUPPORTED;
         }
         return ERROR_SUBNET;
     }

     // 计算网络掩码
     uint32_t mask = (prefix_len == 0) ? 0 : htonl(~((1 << (32 - prefix_len)) - 1));

     // 计算广播地址
     uint32_t ip = ntohl(ip_addr.s_addr);
     uint32_t broadcast_ip = ip | ~mask;
     broadcast_addr->sin_addr.s_addr = htonl(broadcast_ip);
     broadcast_addr->sin_family = AF_INET;

     return ERROR_NONE;
 }

 // 发送魔术包
 ValidationError send_magic_packet(const unsigned char *mac, const struct sockaddr_in *broadcast_addr) {
     // 创建魔术包
     unsigned char magic_packet[MAGIC_PACKET_SIZE];
     memset(magic_packet, 0xFF, 6);  // 6字节 0xFF

     // 重复 MAC 地址 16 次
     for (int i = 0; i < 16; i++) {
         memcpy(&magic_packet[6 + i*6], mac, 6);
     }

     // 创建 UDP socket
     int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
     if (sock < 0) {
         perror("socket");
         return ERROR_SOCKET;
     }

     // 启用广播
     int broadcast_enable = 1;
     if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
         perror("setsockopt");
         close(sock);
         return ERROR_SOCKET;
     }

     // 发送数据包
     ssize_t sent = sendto(sock, magic_packet, sizeof(magic_packet), 0,
                          (struct sockaddr*)broadcast_addr, sizeof(*broadcast_addr));

     close(sock);

     if (sent != sizeof(magic_packet)) {
         perror("sendto");
         return ERROR_SOCKET;
     }

     return ERROR_NONE;
 }

 int main(int argc, char *argv[]) {
     // 参数检查
     if (argc != 3) {
         if (argc == 1) {
             fprintf(stderr, "ValueError: MAC and SubNet\n");
         } else if (argc == 2) {
             // 尝试解析单个参数
             unsigned char mac[6];
             struct sockaddr_in broadcast_addr;

             ValidationError mac_result = parse_mac(argv[1], mac);
             ValidationError subnet_result = get_broadcast_addr(argv[1], &broadcast_addr);

             if (mac_result == ERROR_NONE && subnet_result == ERROR_SUBNET) {
                 fprintf(stderr, "ValueError: SubNet\n");
             } else if (mac_result == ERROR_MAC && subnet_result == ERROR_NONE) {
                 fprintf(stderr, "ValueError: MAC\n");
             } else if (mac_result == ERROR_IPV6_NOT_SUPPORTED || subnet_result == ERROR_IPV6_NOT_SUPPORTED) {
                 fprintf(stderr, "ValueError: IPv6 Not Supported\n");
             } else {
                 fprintf(stderr, "ValueError: MAC and SubNet\n");
             }
         }
         fprintf(stderr, "Usage: %s <MAC_ADDRESS> <SUBNET_CIDR>\n", argv[0]);
         return EXIT_FAILURE;
     }

     // 解析 MAC 地址
     unsigned char mac[6];
     ValidationError mac_error = parse_mac(argv[1], mac);

     // 解析广播地址
     struct sockaddr_in broadcast_addr;
     ValidationError subnet_error = get_broadcast_addr(argv[2], &broadcast_addr);

     // 错误处理
     if (mac_error != ERROR_NONE || subnet_error != ERROR_NONE) {
         if ((mac_error == ERROR_MAC || mac_error == ERROR_NONE) && 
             (subnet_error == ERROR_SUBNET || subnet_error == ERROR_NONE) &&
             !(mac_error == ERROR_NONE && subnet_error == ERROR_NONE)) {
             fprintf(stderr, "ValueError: SubNet\n");
         } else if ((mac_error == ERROR_MAC || mac_error == ERROR_NONE) && 
                    subnet_error == ERROR_IPV6_NOT_SUPPORTED) {
             fprintf(stderr, "ValueError: IPv6 Not Supported\n");
         } else if ((subnet_error == ERROR_SUBNET || subnet_error == ERROR_NONE) && 
                    mac_error == ERROR_MAC) {
             fprintf(stderr, "ValueError: MAC\n");
         } else if (mac_error == ERROR_MAC && subnet_error == ERROR_SUBNET) {
             fprintf(stderr, "ValueError: MAC and SubNet\n");
         } else if (mac_error == ERROR_MAC && subnet_error == ERROR_IPV6_NOT_SUPPORTED) {
             fprintf(stderr, "ValueError: MAC and IPv6 Not Supported\n");
         } else if (mac_error == ERROR_NONE && subnet_error == ERROR_IPV6_NOT_SUPPORTED) {
             fprintf(stderr, "ValueError: IPv6 Not Supported\n");
         }
         fprintf(stderr, "Usage: %s <MAC_ADDRESS> <SUBNET_CIDR>\n", argv[0]);
         return EXIT_FAILURE;
     }

     // 发送魔术包
     ValidationError result = send_magic_packet(mac, &broadcast_addr);
     printf(result == ERROR_NONE ? "Ok\n" : "Error\n");

     return result == ERROR_NONE ? EXIT_SUCCESS : EXIT_FAILURE;
 }