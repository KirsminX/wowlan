use ipnetwork::IpNetwork;
use std::env;
use std::net::{IpAddr, UdpSocket};

/*
 * 错误类型
 * Mac  Mac 地址错误/缺失
 * SubNet   子网掩码错误/缺失
 */
#[derive(Debug, PartialEq)]
enum ValidationError {
    Mac,
    Subnet,
    IpV6NotSupported, 
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let usage = format!("Usage: {} <MAC_ADDRESS> <SUBNET_CIDR>", args.get(0).unwrap_or(&String::from("program")));

    if args.len() != 3 {
        if args.len() == 1 {
            // 没有参数
            println!("ValueError: MAC and SubNet");
            println!("{}", usage);
        } else if args.len() == 2 {
            // 参数不足
            let mac_res = parse_mac(&args[1]);
            let subnet_res = get_broadcast_addr(&args[1]);
            if mac_res.is_ok() && subnet_res.is_err() {
                println!("ValueError: SubNet");
            } else if mac_res.is_err() && subnet_res.is_ok() {
                println!("ValueError: MAC");
            } else {
                println!("ValueError: MAC and SubNet");
            }
            println!("{}", usage);
        } else {
            println!("{}", usage);
        }
        return;
    }

    let mac_str = &args[1];
    let subnet_str = &args[2];

    let mut validation_errors = Vec::new();

    // 验证和解析参数
    let mac_addr = match parse_mac(mac_str) {
        Ok(addr) => Some(addr),
        Err(_) => {
            validation_errors.push(ValidationError::Mac);
            None
        }
    };

    let broadcast_addr = match get_broadcast_addr(subnet_str) {
        Ok(addr) => {
            // 检查是否为IPv4地址
            match addr {
                IpAddr::V4(_) => Some(addr),
                IpAddr::V6(_) => {
                    validation_errors.push(ValidationError::IpV6NotSupported);
                    None
                }
            }
        },
        Err(_) => {
            validation_errors.push(ValidationError::Subnet);
            None
        }
    };

    // 如果有错误，打印错误信息
    if !validation_errors.is_empty() {
        let mut error_parts = Vec::new();
        if validation_errors.contains(&ValidationError::Mac) {
            error_parts.push("MAC");
        }
        if validation_errors.contains(&ValidationError::Subnet) {
            error_parts.push("SubNet");
        }
        if validation_errors.contains(&ValidationError::IpV6NotSupported) {
            error_parts.push("IPv6 Not Supported");
        }
        println!("ValueError: {}", error_parts.join(" and "));
        println!("{}", usage);
        return;
    }
    
    match send_magic_packet(mac_addr.unwrap(), broadcast_addr.unwrap()) {
        Ok(_) => println!("Ok"),
        Err(_) => println!("Error"),
    }
}

/// 解析字符串格式的MAC地址为6字节的数组
fn parse_mac(mac_str: &str) -> Result<[u8; 6], ()> {
    // 首先判断格式类型
    if mac_str.contains(':') || mac_str.contains('-') {
        let parts: Vec<&str> = mac_str.split(|c| c == ':' || c == '-').collect();
        if parts.len() != 6 {
            return Err(());
        }

        let mut mac = [0u8; 6];
        for (i, part) in parts.iter().enumerate() {
            match u8::from_str_radix(part, 16) {
                Ok(byte) => mac[i] = byte,
                Err(_) => return Err(())
            }
        }
        Ok(mac)
    } else if mac_str.len() == 12 {
        // 无分隔符格式
        let mut mac = [0u8; 6];
        for i in 0..6 {
            match u8::from_str_radix(&mac_str[i*2..i*2+2], 16) {
                Ok(byte) => mac[i] = byte,
                Err(_) => return Err(())
            }
        }
        Ok(mac)
    } else {
        Err(())
    }
}

/// 读取广播地址
fn get_broadcast_addr(subnet_str: &str) -> Result<IpAddr, ()> {
    match subnet_str.parse::<IpNetwork>() {
        Ok(network) => Ok(network.broadcast()),
        Err(_) => Err(()),
    }
}

/// 发送 WOL 幻数据包
fn send_magic_packet(mac: [u8; 6], broadcast_addr: IpAddr) -> Result<(), std::io::Error> {
    // 确保是IPv4地址
    let ipv4_addr = match broadcast_addr {
        IpAddr::V4(addr) => addr,
        IpAddr::V6(_) => return Err(std::io::Error::new(std::io::ErrorKind::InvalidInput, "IPv6 addresses are not supported")),
    };

    // 幻数据包
    let mut magic_packet = [0u8; 102];

    //6-bit 0xFF
    magic_packet[0..6].fill(0xFF);

    // 16xMAC 地址
    for i in 0..16 {
        let start = 6 + i * 6;
        magic_packet[start..start+6].copy_from_slice(&mac);
    }

    // 绑定地址 (只使用IPv4)
    let bind_addr = "0.0.0.0:0";

    // UDP
    let socket = UdpSocket::bind(bind_addr)?;

    // 启用广播模式
    socket.set_broadcast(true)?;

    // 尝试发送到多个端口以提高兼容性
    let ports = [9, 7, 0]; // 常见的WOL端口
    
    for port in &ports {
        let dest_addr = (ipv4_addr, *port);
        match socket.send_to(&magic_packet, dest_addr) {
            Ok(_) => {
                // 成功发送到一个端口
                eprintln!("WOL packet sent to port {}", port);
            },
            Err(e) => {
                eprintln!("Failed to send WOL packet to port {}: {}", port, e);
            }
        }
    }

    Ok(())
}