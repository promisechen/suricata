部署代码
=========

根据这个连接一步一步做  \
https://redmine.openinfosecfoundation.org/projects/suricata/wiki/Quick_Start_Guide  


* 安装方法

通过发布版本，可以按照这个链接去安装，但是git下载的代码似乎不行\
我使用的是3.2.1版本 centos7.0上安装的

https://redmine.openinfosecfoundation.org/projects/suricata/wiki/CentOS_Installation  


* 运行方法 

https://redmine.openinfosecfoundation.org/projects/suricata/wiki/Basic_Setup


文件还原的简单实验
===================

本文介绍了，从安装到文件还原的过程。

安装部署
---------

apt-get install vim openssh-server ethtool libpcap-dev libnfnetlink-dev libnetfilter-queue-dev \
libdnet-dev libdumbnet-dev libpcre3-dev libpcre3-dbg bison flex make zlib1g-dev autoconf libtool \
libnss3-dev libnspr4-dev libjansson4 libjansson-dev libyaml-dev libcap-ng0 libcap-ng-dev libnet1-dev libmagic-dev build-essential

获取Suricata的源代码::

    #cd /tmp
    #wget wget http://www.openinfosecfoundation.org/download/suricata-2.0.tar.gz
    #tar zxvf suricata-2.0.tar.gz
    #cd suricata-2.0
    编译与安装:
    #./configure --enable-nfqueue --enable-gccprotect --prefix=/usr/local/suricata --localstatedir=/var
    #make -j3
    #make make-full


配置
-----

编辑配置文件suricata.yaml

* 设置以下2项大一点::

    request-body-limit: 1gb #3072
    response-body-limit: 1gb #3072

* 启动文件保存功能::

    - file-store:      
    enabled: yes       # set to yes to enable      
    log-dir: files    # directory to store the files      
    force-magic: no   # force logging magic on all stored files
    force-md5: no     # force logging of md5 checksums #3.2.1版本没有这个字段，不用填写  
    waldo: file.waldo # waldo file to store the file_id across runs #3.2.1版本没有这个字段，不用填写
    # output module to log files tracked in a easily parsable json format
    - file-log:      
    enabled: yes      
    filename: files-json.log      
    append: yes    #filetype: regular # 'regular', 'unix_stream' or 'unix_dgram' 

* 启动文件保存功能

  增加我们的测试文件( test.rules)到"default-rule-path:", 像下面这样::

  default-rule-path: /usr/local/suricata/etc/suricata/rules
  rule-files:  - test.rules  - botcc.rules
  创建测试规则文件:
  /usr/local/suricata/etc/suricata/rules/test.rules
  加一行测试规则到test.rules，这行规则会保存jpg文件 :
  alert http any any -> any any (msg:"FILESTORE jpg"; fileext:"jpg"; filestore; sid:6; rev:1;)

启动
-----

启动eth1:#ifconfig eth1 up

根据Suricata社区的wiki，我们必须关闭TCP GSO ::
    
    ethtool -K eth1 tso off
    ethtool -K eth1 gro off
    ethtool -K eth1 lro off
    ethtool -K eth1 gso off
    ethtool -K eth1 rx off
    ethtool -K eth1 tx off
    ethtool -K eth1 sg off
    ethtool -K eth1 rxvlan off
    ethtool -K eth1 txvlan off
    ethtool -N eth1 rx-flow-hash udp4 sdfn
    ethtool -N eth1 rx-flow-hash udp6 sdfn
    ethtool -n eth1 rx-flow-hash udp6
    ethtool -n eth1 rx-flow-hash udp4
    ethtool -C eth1 rx-usecs 1000
    ethtool -C eth1 adaptive-rx off

现在可以运行Suricata: /usr/local/suricata/bin/suricata -c /usr/local/suricata/etc/suricata//suricata.yaml -i eth1
在HOST系统上使用Firefox/Chrome访问一些网站，比如这些链接。之后你应该能在/var/log/suricata/files文件夹下面看到美女图片了。感谢Suricata社区的黑客们为自由软件社区所作的贡献。"
