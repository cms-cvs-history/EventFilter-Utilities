#include "FastMonitoringService.h"
#include <iostream>

#include "boost/filesystem.hpp"
#include <iomanip>

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
  	,sleepTime_(iPS.getUntrackedParameter<int>("sleepTime", 1))
    ,rootDirectory_(iPS.getUntrackedParameter<string>("rootDirectory", "/data"))
    ,defPath_(iPS.getUntrackedParameter<string>("definitionPath", "/tmp/def.jsd"))
    ,fastName_(iPS.getUntrackedParameter<string>("fastName", "states"))
    ,slowName_(iPS.getUntrackedParameter<string>("slowName", "lumi"))
    ,firstLumi_(true)
  {
    fmt_.m_data.macrostate_=FastMonitoringThread::sInit;
    fmt_.m_data.ministate_=&nopath_;
    fmt_.m_data.microstate_=&reservedMicroStateNames[mInvalid];
    fmt_.m_data.lumisection_ = 0;
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

    fmt_.m_data.macrostateJ_.setName("Macrostate");
    fmt_.m_data.ministateJ_.setName("Ministate");
    fmt_.m_data.microstateJ_.setName("Microstate");
    fmt_.m_data.processedJ_.setName("Processed");
    vector<JsonMonitorable*> monParams;
    monParams.push_back(&fmt_.m_data.macrostateJ_);
    monParams.push_back(&fmt_.m_data.ministateJ_);
    monParams.push_back(&fmt_.m_data.microstateJ_);
    monParams.push_back(&fmt_.m_data.processedJ_);

    /**
     * MARK! FIND where to output fast and slow files
     */

    // The run dir should be set via the configuration
    // For now, just grab the latest run directory available

    // FIND RUN DIRECTORY
    // TODO refactor this, copied from FedRawDataInputSource
    std::vector<boost::filesystem::path> dirs;
    boost::filesystem::directory_iterator itEnd;
    do {
    	for ( boost::filesystem::directory_iterator it(rootDirectory_);
    			it != itEnd; ++it)
    	{
    		std::cout << " it " << it->path().string() << std::endl;
    		if ( boost::filesystem::is_directory(it->path()) &&
    				std::string::npos != it->path().string().find("run") )
    			dirs.push_back(*it);
    	}
    	if (dirs.empty()) usleep(500000);
    } while ( dirs.empty() );
    std::sort(dirs.begin(), dirs.end());
    boost::filesystem::path runDirectory = dirs.back();

    //FIND host_pid directory
    char thishost[256];
    gethostname(thishost,255);
    std::ostringstream myDir;
    //myDir << std::setfill('0') << std::setw(5) << thishost << "_" << getpid();
    myDir << std::setfill('0') << std::setw(5) << thishost << "_MON";
    boost::filesystem::path workingDirectory_ = runDirectory;
    /*
    boost::filesystem::directory_iterator itEnd2;
    //
    for ( boost::filesystem::directory_iterator it(runDirectory);
    		it != itEnd2; ++it)
    {
    	if ( boost::filesystem::is_directory(it->path()) &&
    			it->path().string().find("/hlt") !=std::string::npos)
    		//foundHLTdir=true;
    }
    */
    workingDirectory_ /= "hlt";
    workingDirectory_ /= myDir.str();

    // now at /root/run/hlt/host_MON

    bool foundMonDir = false;
    if ( boost::filesystem::is_directory(workingDirectory_))
    	foundMonDir=true;
    if (!foundMonDir) {
    	std::cout << "<MON> DIR NOT FOUND!" << std::endl;
        //boost::filesystem::create_directories(workingDirectory_);
    }

    string fastPath, slowPath;
    std::ostringstream fastFileName, slowFileName;

    fastFileName << fastName_ << "_" << getpid() << ".fast";
    boost::filesystem::path fast = workingDirectory_;
    fast /= fastFileName.str();
    fastPath = fast.string();

    slowFileName << slowName_ << "_" << getpid() << ".jsh";
    boost::filesystem::path slow = workingDirectory_;
    slow /= slowFileName.str();
    slowPath = slow.string();

    /*
     * initialize the fast monitor with:
     * vector of pointers to monitorable parameters
     * path to definition
     * output path for the one-liner CSV (fast) file
     * output path for per-lumi JSON histo file
     *
     * number of macrostates
     * number of ministates
     * number of microstates
     *
     */
    std::cout << "FastMonitoringService: initializing FastMonitor with: "
    		<< defPath_ << " " << fastPath << " " << slowPath << " "
    		<< FastMonitoringThread::MCOUNT << " " << encPath_.current_ + 1 << " "
    		<< encModule_.current_ + 1<< std::endl;

    fmt_.m_data.jsonMonitor_.reset(
			new FastMonitor(monParams, defPath_, fastPath, slowPath,
					FastMonitoringThread::MCOUNT, encPath_.current_ + 1,
					encModule_.current_ + 1));

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
	  std::cout << "FastMonitoringService: Pre-begin LUMI: " << iID.luminosityBlock() << std::endl;
	  // MARK! Service .json output per lumi
	  if (firstLumi_)
	  {
		  firstLumi_ = false;
		  return;
	  }

	  fmt_.monlock_.lock();

	  fmt_.m_data.lumisection_ = (unsigned int) iID.luminosityBlock();
	  // TODO remove
	  std::cout << ">>> >>> FastMonitoringService: processed event count for the previous lumi = " << fmt_.m_data.processedJ_.value() << std::endl;
	  fmt_.m_data.jsonMonitor_->snap();
	  fmt_.m_data.jsonMonitor_->outputFullHistoDataPoint(fmt_.m_data.lumisection_ - 1);

	  fmt_.m_data.processedJ_ = 0;
	  fmt_.monlock_.unlock();
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
    fmt_.monlock_.lock();
    fmt_.m_data.processedJ_.value()++;
    fmt_.monlock_.unlock();
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

