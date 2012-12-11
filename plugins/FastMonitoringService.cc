#include "FastMonitoringService.h"
#include <iostream>

namespace evf{

  const std::string FastMonitoringService::macroStateNames[FastMonitoringThread::MCOUNT] = 
    {"Init","JobReady","RunGiven","Running",
     "Stopping","Done","JobEnded","Error","ErrorEnded","End",
     "Invalid"};

  const std::string FastMonitoringService::nopath_ = "NoPath";

  FastMonitoringService::FastMonitoringService(const edm::ParameterSet& iPS, 
				       edm::ActivityRegistry& reg) : 
    MicroStateService(iPS,reg)
    ,encModule_(33)
    ,encPath_(0)
  {
    fmt_.m_data.macrostate_=FastMonitoringThread::sInit;
    fmt_.m_data.ministate_=&nopath_;
    fmt_.m_data.microstate_=&reservedMicroStateNames[mInvalid];
    reg.watchPreModuleBeginJob(this,&FastMonitoringService::preModuleBeginJob);  
    reg.watchPreBeginLumi(this,&FastMonitoringService::preBeginLumi);  
    reg.watchPrePathBeginRun(this,&FastMonitoringService::prePathBeginRun);
    reg.watchPostBeginJob(this,&FastMonitoringService::postBeginJob);
    reg.watchPostBeginRun(this,&FastMonitoringService::postBeginRun);
    reg.watchPostEndJob(this,&FastMonitoringService::postEndJob);
    reg.watchPreProcessPath(this,&FastMonitoringService::preProcessPath);
    reg.watchPreProcessEvent(this,&FastMonitoringService::preEventProcessing);
    reg.watchPostProcessEvent(this,&FastMonitoringService::postEventProcessing);
    reg.watchPreSource(this,&FastMonitoringService::preSource);
    reg.watchPostSource(this,&FastMonitoringService::postSource);
  
    reg.watchPreModule(this,&FastMonitoringService::preModule);
    reg.watchPostModule(this,&FastMonitoringService::postModule);
    reg.watchJobFailure(this,&FastMonitoringService::jobFailure);
    for(unsigned int i = 0; i < (mCOUNT); i++)
      encModule_.updateReserved((void*)(reservedMicroStateNames+i));
    encPath_.update((void*)&nopath_);
    encModule_.completeReservedWithDummies();
    fmt_.start(&FastMonitoringService::dowork,this);
  }


  FastMonitoringService::~FastMonitoringService()
  {
  }

  void FastMonitoringService::preModuleBeginJob(const edm::ModuleDescription& desc)
  {
    //build a map of modules keyed by their module description address
    //here we need to treat output modules in a special way so they can be easily singled out
    if(desc.moduleName() == "ShmStreamConsumer" || desc.moduleName() == "EventStreamFileWriter" || 
       desc.moduleName() == "PoolOutputModule")
      encModule_.updateReserved((void*)&desc);
    else
      encModule_.update((void*)&desc);
  }

  void FastMonitoringService::prePathBeginRun(const std::string& pathName)
  {
    //bonus track, now monitoring path execution too...
    // here we are forced to use string keys...
    encPath_.update((void*)&pathName);
  }

  void FastMonitoringService::postBeginRun(edm::Run const&, edm::EventSetup const&)
  {
    std::cout << "path legenda*****************" << std::endl;
    std::cout << makePathLegenda()   << std::endl;
    fmt_.m_data.macrostate_ = FastMonitoringThread::sRunning;
  }

  void FastMonitoringService::preProcessPath(const std::string& pathName)
  {
    //bonus track, now monitoring path execution too...
    fmt_.m_data.ministate_ = &pathName;
  }

  std::string FastMonitoringService::makePathLegenda(){
    
    std::ostringstream ost;
    for(int i = 0;
	i < encPath_.current_;
	i++)
      ost<<i<<"="<<*((std::string *)(encPath_.decode(i)))<<" ";
    return ost.str();
  }

  std::string FastMonitoringService::makeModuleLegenda(){
    
    std::ostringstream ost;
    for(int i = 0;
	i < encModule_.current_;
	i++)
      {
	//	std::cout << "for i = " << i << std::endl;
	ost<<i<<"="<<((const edm::ModuleDescription *)(encModule_.decode(i)))->moduleLabel()<<" ";
      }
    return ost.str();
  }

  void FastMonitoringService::postBeginJob()
  {
    //    boost::mutex::scoped_lock sl(lock_);
    std::cout << "path legenda*****************" << std::endl;
    std::cout << makePathLegenda()   << std::endl;
    std::cout << "module legenda***************" << std::endl;
    std::cout << makeModuleLegenda() << std::endl;
    fmt_.m_data.macrostate_ = FastMonitoringThread::sJobReady;
  }

  void FastMonitoringService::postEndJob()
  {
    //    boost::mutex::scoped_lock sl(lock_);
    fmt_.m_data.macrostate_ = FastMonitoringThread::sJobEnded;
    fmt_.stop();
  }

  void FastMonitoringService::preBeginLumi(edm::LuminosityBlockID const& iID, edm::Timestamp const& iTime)
  {

  }

  void FastMonitoringService::preEventProcessing(const edm::EventID& iID,
					     const edm::Timestamp& iTime)
  {
    //    boost::mutex::scoped_lock sl(lock_);
  }

  void FastMonitoringService::postEventProcessing(const edm::Event& e, const edm::EventSetup&)
  {
    //    boost::mutex::scoped_lock sl(lock_);
    fmt_.m_data.microstate_ = &reservedMicroStateNames[mFwkOvh];
  }
  void FastMonitoringService::preSource()
  {
    //    boost::mutex::scoped_lock sl(lock_);
    fmt_.m_data.microstate_ = &reservedMicroStateNames[mIdle];
  }

  void FastMonitoringService::postSource()
  {
    //    boost::mutex::scoped_lock sl(lock_);
    fmt_.m_data.microstate_ = &reservedMicroStateNames[mFwkOvh];
  }

  void FastMonitoringService::preModule(const edm::ModuleDescription& desc)
  {
    //    boost::mutex::scoped_lock sl(lock_);
    fmt_.m_data.microstate_ = &desc;
  }

  void FastMonitoringService::postModule(const edm::ModuleDescription& desc)
  {
    //    boost::mutex::scoped_lock sl(lock_);
    fmt_.m_data.microstate_ = &desc;
  }
  void FastMonitoringService::jobFailure()
  {
    //    boost::mutex::scoped_lock sl(lock_);
    fmt_.m_data.macrostate_ = FastMonitoringThread::sError;
  }
  void FastMonitoringService::setMicroState(Microstate m)
  {
    //    boost::mutex::scoped_lock sl(lock_);
    fmt_.m_data.microstate_ = &reservedMicroStateNames[m];
  }

} //end namespace evf

