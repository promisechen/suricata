利用周末空余时间，将dpdk集成到suricata3.2.1中，没有考虑性能，也没有考虑配置（只能监听0号端口，核心也无法配置，默认使用一个核心）。只是为了演示一个思路、练习下实战经验。   

本示例在pcap-file方式的基础上做了修改，使其可以兼容dpdk.注意：必须以pcap-file的方式启动，才能使用dpdk模式，并且将会先读取离线报文，在接受dpdk 0号端口的报文。     

1. 安装dpdk16.07版本    
    根据官方手册，安装部署，绑定网卡等   
    将libintel_dpdk.so放到/lib64/或者其他目录下   
2. 安装suricata3.2.1   
    a.直接下载该git   

    b.下载官方的3.2.1版本，然后从本目录下找到xxx.patch文件   
    
        将patch打入版本中     
        
        根据官方指导手册或参考https://github.com/promisechen/suricata/blob/master/doc/source/quick_start.rst
        
        注意：应该指定dpdk相关目录 ./configure --with-libdpdk-includes=/root/work/wlan/dpdk/x86_64-native-linuxapp-gcc/include/ --with-libdpdk-libraries=/lib64/      

3. 启动测试，观察效果    

        suricata -c /usr/local/suricata/etc/suricata/suricata.yaml -r /root/pcap/http.pcap 
        
        可以到/var/log/suricata中查看

        日志示例: 

```
{"timestamp":"1900-01-00T00:00:00.000000+0000","flow_id":1896226812262547,"event_type":"flow","src_ip":"101.6.6.178","src_port":80,"dest_ip":"172.16.44.154","dest_port":42491,"proto":"TCP","flow":{"pkts_toserver":31,"pkts_toclient":0,"bytes_toserver":44702,"bytes_toclient":0,"start":"2017-04-22T00:38:38.653459+0800","end":"1900-01-00T00:00:00.000000+0000","age":-1492792718,"state":"new","reason":"shutdown","alerted":false},"tcp":{"tcp_flags":"00","tcp_flags_ts":"00","tcp_flags_tc":"00"}}

{"timestamp":"2017-09-10T22:13:13.218316+0800","event_type":"stats","stats":{"uptime":1,"decoder":{"pkts":31,"bytes":44702,"invalid":0,"ipv4":31,"ipv6":0,"ethernet":31,"raw":0,"null":0,"sll":0,"tcp":31,"udp":0,"sctp":0,"icmpv4":0,"icmpv6":0,"ppp":0,"pppoe":0,"gre":0,"vlan":0,"vlan_qinq":0,"teredo":0,"ipv4_in_ipv6":0,"ipv6_in_ipv6":0,"mpls":0,"avg_pkt_size":1442,"max_pkt_size":1442,"erspan":0,"ipraw":{"invalid_ip_version":0},"ltnull":{"pkt_too_small":0,"unsupported_type":0},"dce":{"pkt_too_small":0}},"flow":{"memcap":0,"spare":10000,"emerg_mode_entered":0,"emerg_mode_over":0,"tcp_reuse":0,"memuse":7074592},"defrag":{"ipv4":{"fragments":0,"reassembled":0,"timeouts":0},"ipv6":{"fragments":0,"reassembled":0,"timeouts":0},"max_frag_hits":0},"tcp":{"sessions":0,"ssn_memcap_drop":0,"pseudo":0,"pseudo_failed":0,"invalid_checksum":0,"no_flow":0,"syn":0,"synack":0,"rst":0,"segment_memcap_drop":0,"stream_depth_reached":0,"reassembly_gap":0,"memuse":1638400,"reassembly_memuse":12332832},"detect":{"alert":0},"app_layer":{"flow":{"http":0,"ftp":0,"smtp":0,"tls":0,"ssh":0,"imap":0,"msn":0,"smb":0,"dcerpc_tcp":0,"dns_tcp":0,"failed_tcp":0,"dcerpc_udp":0,"dns_udp":0,"failed_udp":0},"tx":{"http":0,"smtp":0,"tls":0,"dns_tcp":0,"dns_udp":0}},"flow_mgr":{"closed_pruned":0,"new_pruned":0,"est_pruned":0,"bypassed_pruned":0,"flows_checked":1,"flows_notimeout":1,"flows_timeout":0,"flows_timeout_inuse":0,"flows_removed":0,"rows_checked":65536,"rows_skipped":65535,"rows_empty":0,"rows_busy":0,"rows_maxlen":1},"dns":{"memuse":0,"memcap_state":0,"memcap_global":0},"http":{"memuse":0,"memcap":0}}}

```  

