源码笔记
=========

调试
-----------

   ::   

    set arg -c /etc/suricata/suricata.yaml -r /root/work/pcap/txt_simple_syn2get200.pcap 
    #0  TmThreadsSlotVarRun (tv=tv@entry=0x24298b0, p=p@entry=0x7fffec26bf10, slot=slot@entry=0x5d33290) at tm-threads.c:118
    #1  0x000000000053794d in TmThreadsSlotProcessPkt (p=0x7fffec26bf10, s=0x5d33290, tv=0x24298b0) at tm-threads.h:149
    #2  PcapFileCallbackLoop (user=0x7fffec26c8c0 "", h=<optimized out>, pkt=0x7fffec26cdf0 "") at source-pcap-file.c:178
    #3  0x00007ffff746bf89 in pcap_offline_read () from /lib64/libpcap.so.1
    #4  0x0000000000537e06 in ReceivePcapFileLoop (tv=0x24298b0, data=0x7fffec26c8c0, slot=<optimized out>) at source-pcap-file.c:211
    #5  0x0000000000552c92 in TmThreadsSlotPktAcqLoop (td=0x24298b0) at tm-threads.c:334
    #6  0x00007ffff5d91df3 in start_thread () from /lib64/libpthread.so.0
    #7  0x00007ffff58bb3dd in clone () from /lib64/libc.so.6

tcp识别总体流程
----------------------

FlowWorker->       
   StreamTcp      
          ->AppLayerHandleTCPData     
               ->  TCPProtoDetect    
               ->  AppLayerParserParse     

   Detect    
       -> SigMatchSignatures    
           -> PrefilterTx    

* 先经过StreamTcp进行流重组、协议识别、报文结构解析     
   	* 首先进行重组   
   	* 重组完成后,调用AppLayerHandleTCPData进行应用层处理    
	    *  调用TCPProtoDetect进行协议识别    
	    *  调用AppLayerParserParse进行协议解析(比如http解析锚点、html头部等)，协议解析见下文描述。    
		
* Detect进行业务识别(SigMatchSignatures)       
   	* 经过上一步骤已经完成了应用层的解码。
   	* 后续可以根据uri\host\user-agent等进行协议识别  	   	


flow模块
----------
流表采用加锁的方式，有专门的释放流表的线程.                                                            
FlowWorker->DetectNoFlow->SigMatchSignatures->DeStateDetectStartDetection->DetectFileInspectHttp->DetectFileInspect->DetectFileextMatch

TCP重组
----------

DoReassemble 是重组完成后，调用
  
乱序的时候: StreamTcpReassembleAppLayer ->DoReassemble->AppLayerHandleTCPData   

无乱序的时候: StreamTcpReassembleAppLayer ->AppLayerHandleTCPData   
  
无论是否分段，StreamTcpReassembleAppLayer是重组的最后一个主要函数，如果有乱序重组后将调用DoReassemble。

该部分也不是我最关心的的了。   

协议识别
---------

alpd_ctx是协议识别的全局变量，存放了各种协议识别使用的数据：如字符串，状态机等

协议识别--识别过程 
AppLayerProtoDetectGetProto -> AppLayerProtoDetectPMGetProto:通过特征串匹配  ，这个就是一个匹配，代码也不多
                            -> AppLayerProtoDetectPPGetProto:通过探测方式识别，主动发送报文探测结果  

AppLayerParserParse；这个没有在分析                         

协议识别--初始化过程(特征串方式):                           
AppLayerSetup -> AppLayerProtoDetectSetup:初始化单模多模算法等
                 AppLayerParserRegisterProtocolParsers:各协议添加字符串,会调用个协议的RegisterxxxxxParsers.
                        他们最终都会调用AppLayerProtoDetectPMRegisterPatternCS 和
                        AppLayerProtoDetectPMRegisterPatternCI注册字符串,而这两个函数最终都会掉AppLayerProtoDetectPMAddSignature。
                        比如:RegisterHTPParsers->HTPRegisterPatternsForProtocolDetection ->AppLayerProtoDetectPMRegisterPatternCI                        
                        
                AppLayerProtoDetectPrepareState：添加特征到状态机并编译
                        ->AppLayerProtoDetectPMMapSignatures :添加到状态机
                        ->AppLayerProtoDetectPMPrepareMpm :编译

协议识别--初始化过程(探测方式): 

通过如下调用，将注册探测函数。如RegisterDNSUDPParsers中注册了DNSUdpProbingParser探测函数
AppLayerProtoDetectPPParseConfPorts->AppLayerProtoDetectPPRegister                                                  
AppLayerProtoDetectPPRegister->AppLayerProtoDetectInsertNewProbingParser->AppLayerProtoDetectProbingParserElementDuplicate

应用识别   
----------   
   
感觉基本很像snort。发现这个与我看suricata的目的，没有任何关系了,果断停止。  
  以uri的检测方式，简单说明下吧。    
  DetectHttpUriRegister注册uri的识别方法。最终进行识别的是PrefilterTxUri和DetectEngineInspectHttpUri进行的。  
      
* Detect进行业务识别(SigMatchSignatures)            
   	* 先调用SigMatchSignatures     
   	* 在依次调用Prefilter和PrefilterTx      
         在PrefilterTx 中调用AppLayerParserGetTx（StateGetTx） 获取到识别需要用到的相关字段值，在调用PrefilterTx进行相应方法的识别。  
         这里是个重点, 对协议解析部分分析的时候会对StateGetTx详细阐述。       
   	* 在调用PrefilterTxUri和DetectEngineInspectHttpUri        
          通过streamTcp之后，就会把http的头部信息解析完了，会将uri传到这个PrefilterTxUri函数中。      

协议解析
----------


最终到应用层都会调用AppLayerHandleTCPData，AppLayerHandleTCPData函数是重组有序之后第一个被调用的函数，这里将都是有序报文。

对于http的文件还原，使用了libhtp这个库。目前看是先经过tcpstream进行流重组，然后送给libhtp进行解析，最后在回调到http模块生成文件。
最终涉及的两个函数HTPFileOpen(创建文件)和HTPFileStoreChunk(追加到文件中)。 
    
    HTPFileOpen调用过程 :: 

        #0  HTPFileOpen (s=s@entry=0x7fffe4099ac0, filename=0x7fffe409a8c8 "/web/a.txt", filename_len=10, 
        data=data@entry=0x7ffff227140b "sdfasdfasdfasdf\nasdfasdf\nasdfasdf\nasdf\nasd\nf\nasd\nfa\ndsf\nas\ndf\niiiasd\n@\216B\002", 
        data_len=data_len@entry=69, txid=0, direction=direction@entry=8 '\b') at app-layer-htp-file.c:81
        #1  0x000000000044cd38 in HtpResponseBodyHandle (hstate=hstate@entry=0x7fffe4099ac0, htud=htud@entry=0x7fffe409a910, tx=0x7fffe4099e60, 
        data=0x7ffff227140b "sdfasdfasdfasdf\nasdfasdf\nasdfasdf\nasdf\nasd\nf\nasd\nfa\ndsf\nas\ndf\niiiasd\n@\216B\002", data_len=69)
        at app-layer-htp.c:1653
        #2  0x000000000044cede in HTPCallbackResponseBodyData (d=0x7ffff2271070) at app-layer-htp.c:1866
        #3  0x00007ffff76930b6 in htp_hook_run_all (hook=0x86b210, user_data=0x7ffff2271070) at htp_hooks.c:127
        #4  0x00007ffff76a4d06 in htp_res_run_hook_body_data (connp=0x7fffe4099b20, d=0x7ffff2271070) at htp_util.c:2430
        #5  0x00007ffff769e6f2 in htp_tx_res_process_body_data_ex (tx=0x7fffe4099e60, data=0x7ffff227140b, len=69) at htp_transaction.c:836
        #6  0x00007ffff769a1ad in htp_connp_RES_BODY_IDENTITY_CL_KNOWN (connp=0x7fffe4099b20) at htp_response.c:462
        #7  0x00007ffff769b893 in htp_connp_res_data (connp=0x7fffe4099b20, timestamp=0x7ffff2271110, data=0x7ffff2271298, len=440) at htp_response.c:1084
        #8  0x000000000044b906 in HTPHandleResponseData (f=<optimized out>, htp_state=0x7fffe4099ac0, pstate=0x7fffe4099a90, input=<optimized out>, 
        input_len=<optimized out>, local_data=<optimized out>) at app-layer-htp.c:827
        #9  0x0000000000453156 in AppLayerParserParse (tv=tv@entry=0x2428e20, alp_tctx=<optimized out>, f=f@entry=0x14bb5b0, alproto=1, 
        flags=flags@entry=11 '\v', 
        input=input@entry=0x7ffff2271298 "HTTP/1.1 200 OK\r\nDate: Wed, 22 Feb 2017 06:01:32 GMT\r\nServer: Apache/2.4.6 (CentOS) OpenSSL/1.0.1e-fips mod_fcgid/2.3.9 PHP/5.4.16 mod_wsgi/3.4 Python/2.7.5\r\nLast-Modified: Wed, 22 Feb 2017 05:57:07 G"..., input_len=input_len@entry=440)
        at app-layer-parser.c:998
        #10 0x0000000000414649 in TCPProtoDetect (flags=11 '\v', data_len=440, 
        data=0x7ffff2271298 "HTTP/1.1 200 OK\r\nDate: Wed, 22 Feb 2017 06:01:32 GMT\r\nServer: Apache/2.4.6 (CentOS) OpenSSL/1.0.1e-fips mod_fcgid/2.3.9 PHP/5.4.16 mod_wsgi/3.4 Python/2.7.5\r\nLast-Modified: Wed, 22 Feb 2017 05:57:07 G"..., stream=0x7ffff2271298, ssn=0x7fffe407a190, f=0x14bb5b0, 
        p=<optimized out>, app_tctx=<optimized out>, ra_ctx=0x7fffe4001720, tv=<optimized out>) at app-layer.c:446
        #11 AppLayerHandleTCPData (tv=tv@entry=0x2428e20, ra_ctx=ra_ctx@entry=0x7fffe4001720, p=p@entry=0x95ffd40, f=0x14bb5b0, 
        ssn=ssn@entry=0x7fffe407a190, stream=stream@entry=0x7fffe407a1a0, 
        data=data@entry=0x7ffff2271298 "HTTP/1.1 200 OK\r\nDate: Wed, 22 Feb 2017 06:01:32 GMT\r\nServer: Apache/2.4.6 (CentOS) OpenSSL/1.0.1e-fips mod_fcgid/2.3.9 PHP/5.4.16 mod_wsgi/3.4 Python/2.7.5\r\nLast-Modified: Wed, 22 Feb 2017 05:57:07 G"..., data_len=440, flags=11 '\v')
        at app-layer.c:590
        #12 0x000000000054581b in StreamTcpReassembleAppLayer (tv=tv@entry=0x2428e20, ra_ctx=ra_ctx@entry=0x7fffe4001720, ssn=ssn@entry=0x7fffe407a190, 
        stream=stream@entry=0x7fffe407a1a0, p=p@entry=0x95ffd40) at stream-tcp-reassemble.c:3068
        #13 0x0000000000546161 in StreamTcpReassembleHandleSegmentUpdateACK (tv=tv@entry=0x2428e20, ra_ctx=ra_ctx@entry=0x7fffe4001720, 
        ssn=ssn@entry=0x7fffe407a190, stream=stream@entry=0x7fffe407a1a0, p=p@entry=0x95ffd40) at stream-tcp-reassemble.c:3419
        #14 0x0000000000547b10 in StreamTcpReassembleHandleSegment (tv=tv@entry=0x2428e20, ra_ctx=0x7fffe4001720, ssn=ssn@entry=0x7fffe407a190, 
        stream=0x7fffe407a1e8, p=p@entry=0x95ffd40, pq=pq@entry=0x7fffe40008e0) at stream-tcp-reassemble.c:3447
        #15 0x000000000054082c in StreamTcpPacket (tv=0x2428e20, p=0x95ffd40, stt=0x7fffe4001440, pq=0x7fffe40008e0) at stream-tcp.c:4515
        #16 0x00000000005426ea in StreamTcp (tv=0x7fffe4099ac0, tv@entry=0x2428e20, p=0x7fffe409a8c8, p@entry=0x95ffd40, data=0xa, pq=0x7ffff227140b, 
        pq@entry=0x7fffe40008e0, postpq=0x45, postpq@entry=0x0) at stream-tcp.c:4918
        #17 0x00000000004fa309 in FlowWorker (tv=0x2428e20, p=0x95ffd40, data=0x7fffe40008c0, preq=0x2428f70, unused=<optimized out>) at flow-worker.c:194
        #18 0x0000000000550824 in TmThreadsSlotVarRun (tv=tv@entry=0x2428e20, p=p@entry=0x95ffd40, slot=slot@entry=0x2428f30) at tm-threads.c:128
        #19 0x0000000000553275 in TmThreadsSlotVar (td=0x2428e20) at tm-threads.c:585
        #20 0x00007ffff5d89df3 in start_thread () from /lib64/libpthread.so.0
        #21 0x00007ffff58b33dd in clone () from /lib64/libc.so.6
    
    HTPFileStoreChunk调用过程 ::

        #0  HTPFileStoreChunk (s=0x7fffe4099ac0, 
        data=data@entry=0x1affa00 "\355\320(\a\035\236err\244\251\226[S5\374\255\221*\325j\220\273_'r\347\365\060mf\221\266\377\247\366ArL\256b\345\366c\264\033\002\\\004\200.\212%\267C\214\262ʯ\261\030\322dm\216\035.\347\336_\267\355\030\304\357\227\353\376\212\223\340&\356\363\\\234\023J[Iu\234\003\203", data_len=data_len@entry=1412, direction=direction@entry=8 '\b') at app-layer-htp-file.c:194
        #1  0x000000000044cd7f in HtpResponseBodyHandle (hstate=hstate@entry=0x7fffe4099ac0, htud=htud@entry=0x7fffe40a5ee0, tx=<optimized out>, 
        data=0x1affa00 "\355\320(\a\035\236err\244\251\226[S5\374\255\221*\325j\220\273_'r\347\365\060mf\221\266\377\247\366ArL\256b\345\366c\264\033\002\\\004\200.\212%\267C\214\262ʯ\261\030\322dm\216\035.\347\336_\267\355\030\304\357\227\353\376\212\223\340&\356\363\\\234\023J[Iu\234\003\203", data_len=1412) at app-layer-htp.c:1672
        #2  0x000000000044cede in HTPCallbackResponseBodyData (d=0x7ffff22710b0) at app-layer-htp.c:1866
        #3  0x00007ffff76930b6 in htp_hook_run_all (hook=0x86b200, user_data=0x7ffff22710b0) at htp_hooks.c:127
        #4  0x00007ffff76a4d06 in htp_res_run_hook_body_data (connp=0x7fffe4099b20, d=0x7ffff22710b0) at htp_util.c:2430
        #5  0x00007ffff769e6f2 in htp_tx_res_process_body_data_ex (tx=0x7fffe40a53e0, data=0x1affa00, len=1412) at htp_transaction.c:836
        #6  0x00007ffff769a1ad in htp_connp_RES_BODY_IDENTITY_CL_KNOWN (connp=0x7fffe4099b20) at htp_response.c:462
        #7  0x00007ffff769b893 in htp_connp_res_data (connp=0x7fffe4099b20, timestamp=0x7ffff2271150, data=0x1affa00, len=1412) at htp_response.c:1084
        #8  0x000000000044b906 in HTPHandleResponseData (f=<optimized out>, htp_state=0x7fffe4099ac0, pstate=0x7fffe4099a90, input=<optimized out>, 
        input_len=<optimized out>, local_data=<optimized out>) at app-layer-htp.c:827
        #9  0x0000000000453156 in AppLayerParserParse (tv=tv@entry=0x2428df0, alp_tctx=<optimized out>, f=0x14bb5a0, alproto=1, flags=flags@entry=8 '\b', 
        input=0x1affa00 "\355\320(\a\035\236err\244\251\226[S5\374\255\221*\325j\220\273_'r\347\365\060mf\221\266\377\247\366ArL\256b\345\366c\264\033\002\\\004\200.\212%\267C\214\262ʯ\261\030\322dm\216\035.\347\336_\267\355\030\304\357\227\353\376\212\223\340&\356\363\\\234\023J[Iu\234\003\203", input_len=1412) at app-layer-parser.c:998
        #10 0x000000000041468e in AppLayerHandleTCPData (tv=tv@entry=0x2428df0, ra_ctx=ra_ctx@entry=0x7fffe4001720, p=0x1affa00, p@entry=0x7fffec24aa40, 
        f=<optimized out>, ssn=ssn@entry=0x7fffe407a190, stream=stream@entry=0x7fffe407a1a0, data=<optimized out>, data_len=1412, flags=8 '\b')
        at app-layer.c:610
        #11 0x000000000054595b in DoReassemble (p=0x7fffec24aa40, rd=0x7ffff2271250, seg=0x1aff9d0, stream=0x7fffe407a1a0, ssn=0x7fffe407a190, 
        ra_ctx=0x7fffe4001720, tv=0x2428df0) at stream-tcp-reassemble.c:2673
        #12 StreamTcpReassembleAppLayer (tv=tv@entry=0x2428df0, ra_ctx=ra_ctx@entry=0x7fffe4001720, ssn=ssn@entry=0x7fffe407a190, 
        stream=stream@entry=0x7fffe407a1a0, p=p@entry=0x7fffec24aa40) at stream-tcp-reassemble.c:3043
        #13 0x0000000000546161 in StreamTcpReassembleHandleSegmentUpdateACK (tv=tv@entry=0x2428df0, ra_ctx=ra_ctx@entry=0x7fffe4001720, 
        ssn=ssn@entry=0x7fffe407a190, stream=stream@entry=0x7fffe407a1a0, p=p@entry=0x7fffec24aa40) at stream-tcp-reassemble.c:3419
        #14 0x0000000000547b10 in StreamTcpReassembleHandleSegment (tv=tv@entry=0x2428df0, ra_ctx=0x7fffe4001720, ssn=ssn@entry=0x7fffe407a190, 
        stream=0x7fffe407a1e8, p=p@entry=0x7fffec24aa40, pq=pq@entry=0x7fffe4001448) at stream-tcp-reassemble.c:3447
        #15 0x000000000053dd59 in HandleEstablishedPacketToClient (stt=<optimized out>, pq=<optimized out>, p=<optimized out>, ssn=<optimized out>, 
        tv=<optimized out>) at stream-tcp.c:2232
        #16 StreamTcpPacketStateEstablished (tv=tv@entry=0x2428df0, p=p@entry=0x7fffec24aa40, stt=stt@entry=0x7fffe4001440, ssn=ssn@entry=0x7fffe407a190, 
        pq=pq@entry=0x7fffe4001448) at stream-tcp.c:2476
        #17 0x0000000000541027 in StreamTcpPacket (tv=0x2428df0, p=0x7fffec24aa40, stt=0x7fffe4001440, pq=0x7fffe40008e0) at stream-tcp.c:4555
        #18 0x00000000005426ea in StreamTcp (tv=0x7fffe4099ac0, tv@entry=0x2428df0, p=0x1affa00, p@entry=0x7fffec24aa40, data=0x584, pq=0x8, 
        pq@entry=0x7fffe40008e0, postpq=0x584, postpq@entry=0x0) at stream-tcp.c:4918
        #19 0x00000000004fa309 in FlowWorker (tv=0x2428df0, p=0x7fffec24aa40, data=0x7fffe40008c0, preq=0x2428f40, unused=<optimized out>)
        at flow-worker.c:194
        #20 0x0000000000550824 in TmThreadsSlotVarRun (tv=tv@entry=0x2428df0, p=p@entry=0x7fffec24aa40, slot=slot@entry=0x2428f00) at tm-threads.c:128
        #21 0x0000000000553275 in TmThreadsSlotVar (td=0x2428df0) at tm-threads.c:585
        #22 0x00007ffff5d89df3 in start_thread () from /lib64/libpthread.so.0
        #23 0x00007ffff58b33dd in clone () from /lib64/libc.so.6

