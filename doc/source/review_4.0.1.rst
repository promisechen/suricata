./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-nfqueue --enable-lua

当前进度：开始分析AppLayerSetup->AppLayerParserRegisterProtocolParsers->RegisterHTPParsers


1. 主线    

   SCInstanceInit(初始化实例)->SC_ATOMIC_INIT->SCLogInitLogModule(创建日志子系统实例)->SCSetThreadName（设置线程名称）-》ParseSizeInit(应该是解析用的，后续研究)->RunModeRegisterRunModes
   ->ConfInit(创建一个root节点,存储配置信息)->ParseCommandLine(解析命令信息，存放到SCInstance suri局部变量中)->FinalizeRunMode(检查和设定运行模式)->StartInternalRunMode(处理内部模式，比如查看当前支持的协议列表、支持的模式等)
   ->GlobalsInitPreConfig(1.2 初始化全局配置 如队列、快速匹配链表等)－>LoadYamlConfig(1.3  读取yaml格式配置文件)
   ->PostConfLoadedSetup 
   1.1 RunModeRegisterRunModes调用RunModeIdsXXXXRegister将各种模式注册好(todo:以pcap的模式作为模板进行研究)
   他们都调用RunModeRegisterNewRunMode紧张注册，

   1.2 GlobalsInitPreConfig 初始化trans_q 和data_queues（todo:深入分析两个变量） CreateLowercaseTable（字母转换数组初始化） TimeInit SupportFastPatternForSigMatchTypes三个函数逐个调用。
       
       SupportFastPatternForSigMatchTypes将DETECT_SM_LIST_PMATCH加入sm_fp_support_smlist_list链表，优先级是3 
   1.3 todo: 
   1.4
    MpmTableSetup(注册多模式匹配算法)->SpmTableSetup(注册单模式匹配算法)->网卡offloading、checksum等配置读取->AppLayerSetup
     MpmTableSetup:注册各种多模匹配算法，将ac ac-cuda ac_bs ac_tile hyperscan 这几种多模式匹配算法，注册到mpm_table(结构为MpmTableElmt)
      
      全局变量中 mpm_default_matcher作为默认配置

      SpmTableSetup: 注册各种单模匹配算法，将bm hyperscan这两种单模式匹配算法，注册到spm_table(结构为SpmTableElmt)的全局变量中

      AppLayerSetup: 
               AppLayerProtoDetectSetup:主要是对alpd_ctxl4层协议(tcp,udp,icmp,sctp)层面的多模和单模的注册和初始化，主要是给alpd_ctx.spm_global_thread_ctx和MpmInitCtx调用进行赋值(todo:多模匹配算法插件接口)
                                         alpd_ctx是协议识别的全局变量，存放了各种协议识别使用的数据：如字符串，状态机等
             AppLayerParserSetup: 
             AppLayerParserRegisterProtocolParsers:
               


