./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-nfqueue --enable-lua

1. 主线    

   SCInstanceInit(初始化实例)->SC_ATOMIC_INIT->SCLogInitLogModule(创建日志子系统实例)->SCSetThreadName（设置线程名称）-》ParseSizeInit(应该是解析用的，后续研究)->RunModeRegisterRunModes
   ->ConfInit(创建一个root节点,存储配置信息)->ParseCommandLine(解析命令信息，存放到SCInstance suri局部变量中)->FinalizeRunMode(检查和设定运行模式)->StartInternalRunMode(处理内部模式，比如查看当前支持的协议列表、支持的模式等)

   1.1 RunModeRegisterRunModes调用RunModeIdsXXXXRegister将各种模式注册好(todo:以pcap的模式作为模板进行研究)
   他们都调用RunModeRegisterNewRunMode紧张注册，

   1.2 
