#include "EvFDaqDirector.h"
#include "FWCore/Utilities/interface/Exception.h"
#include <iostream>

namespace evf{

  EvFDaqDirector::EvFDaqDirector( const edm::ParameterSet &pset, edm::ActivityRegistry& reg )
    : base_dir_(pset.getUntrackedParameter<std::string>("hltBaseDir","/tmp/hlt"))
    , run_dir_("0")
    , bu_base_dir_(pset.getUntrackedParameter<std::string>("buBaseDir","bu"))
    , sm_base_dir_(pset.getUntrackedParameter<std::string>("smBaseDir","sm"))
    , monitor_base_dir_(pset.getUntrackedParameter<std::string>("monBaseDir","mon"))
    , directorBu_(pset.getUntrackedParameter<bool>("directorIsBu",false))
    , bu_w_flk({F_WRLCK,SEEK_SET,0,0,0})
    , bu_r_flk({F_RDLCK,SEEK_SET,0,0,0})
    , bu_w_fulk({F_UNLCK,SEEK_SET,0,0,0})
    , bu_r_fulk({F_UNLCK,SEEK_SET,0,0,0})
    , fu_rw_flk({F_RDLCK,SEEK_SET,0,0,0})
    , fu_rw_fulk({F_UNLCK,SEEK_SET,0,0,0})
    , bu_w_lock_stream(0)
    , bu_r_lock_stream(0)
    , fu_rw_lock_stream(0)
    , dirManager_(base_dir_)
  {
    reg.watchPreBeginRun(this,&EvFDaqDirector::preBeginRun);  
    reg.watchPostEndRun(this,&EvFDaqDirector::postEndRun);  

    // check if base dir exists or create it accordingly
    int retval = mkdir(base_dir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(retval!=0 && errno !=EEXIST) 
      {
	throw cms::Exception("DaqDirector") << " Error creating base dir " << base_dir_ << " mkdir error:" << strerror(errno) << "\n";
      }

  }

  void  EvFDaqDirector::postEndRun(edm::Run const& run, edm::EventSetup const& es)
  {
    close(bu_readlock_fd_);
    close(bu_writelock_fd_);
    if(directorBu_){
      std::string filename = bu_base_dir_+"/bu.lock";
      removeFile(filename);
    }
  }
  void  EvFDaqDirector::preBeginRun(edm::RunID const& id, edm::Timestamp const& ts)
  {
    std::cout << "DaqDirector - preBeginRun " << std::endl;
    std::ostringstream ost;
    
    
    // check if run dir exists. 
    ost<<base_dir_ << "/run" << id.run();
    run_dir_= ost.str();
    int retval = mkdir(run_dir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(retval!=0 && errno !=EEXIST)
      {
	throw cms::Exception("DaqDirector") << " Error creating run dir " << run_dir_ << " mkdir error:" << strerror(errno) << "\n";
      }
    ost.clear();
    ost.str("");
    // check that the requested run is the latest one - this must be imposed at all times 
    if(dirManager_.findHighestRunDir() != run_dir_){
      throw cms::Exception("DaqDirector") << " Error checking run dir " << run_dir_ << " this is not the highest run "
					  << dirManager_.findHighestRunDir() << "\n";
    }
    //make or find bu base dir
    if (bu_base_dir_.empty()) {
      ost<<base_dir_ << "/run" << id.run();
      bu_base_dir_= ost.str();
    }
    else {
      ost<<base_dir_ << "/run" << id.run() << "/" << bu_base_dir_;
      bu_base_dir_= ost.str();
      retval = mkdir(bu_base_dir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if(retval!=0 && errno !=EEXIST)
        {
	  throw cms::Exception("DaqDirector") << " Error creating bu dir " << bu_base_dir_ << " mkdir error:" << strerror(errno) << "\n";
        }
    }
    ost << "/open";
    bu_base_open_dir_ = ost.str();
    retval = mkdir(bu_base_open_dir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(retval!=0 && errno !=EEXIST)
      {
	throw cms::Exception("DaqDirector") << " Error creating bu dir " << bu_base_open_dir_ << " mkdir error:" << strerror(errno) << "\n";
      }
    ost.clear();
    ost.str("");
    
    //make or find sm base dir
    //retval = mkdir(sm_base_dir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //if(retval!=0 && errno !=EEXIST)
      //{
	//throw cms::Exception("DaqDirector") << " Error creating sm dir " << sm_base_dir_ << " mkdir error:" << strerror(errno) << "\n";
      //}
    ////make run dir for sm
    //ost<< sm_base_dir_ << "/" << id.run();
    //sm_base_dir_= ost.str();
    //retval = mkdir(sm_base_dir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    //if(retval!=0 && errno !=EEXIST)
      //{
	//throw cms::Exception("DaqDirector") << " Error creating sm dir " << sm_base_dir_ << " mkdir error:" << strerror(errno) << "\n";
      //}
    //ost.clear();
    //ost.str("");
    
    //make or find monitor base dir
    ost<<base_dir_ << "/run" << id.run() << "/" << monitor_base_dir_;
    monitor_base_dir_= ost.str();
    retval = mkdir(monitor_base_dir_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(retval!=0 && errno !=EEXIST)
      {
	throw cms::Exception("DaqDirector") << " Error creating monitor dir " << monitor_base_dir_ << " mkdir error:" << strerror(errno) << "\n";
      }
    //create or access  lock files: only the BU director is allowed to create a bu lock, only the FU director is allowed 
    // to create a fu lock
    std::string bulockfile = bu_base_dir_;
    bulockfile += "/bu.lock";
    std::string fulockfile = bu_base_dir_;
    fulockfile += "/fu.lock";

    if(directorBu_){

      // the BU director does not need to know about the fu lock
      bu_writelock_fd_ = open(bulockfile.c_str(),O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
      if(bu_writelock_fd_==-1) std::cout << "problem with creating filedesc for buwritelock " 
					 << strerror(errno) << "\n";
      else
	std::cout << "creating filedesc for buwritelock " 
		  << bu_writelock_fd_ << "\n";
      bu_w_lock_stream = fdopen( bu_writelock_fd_,"w");
      if(bu_w_lock_stream==0)
	std::cout << "Error creating write lock stream " << strerror(errno) << std::endl;
      std::string filename = monitor_base_dir_ + "/bumonitor.txt";
      bu_w_monitor_stream = fopen(filename.c_str(),"w+");
      filename = monitor_base_dir_ + "/diskmonitor.txt";
      bu_t_monitor_stream = fopen(filename.c_str(),"w+");
      if(bu_t_monitor_stream==0)
	std::cout << "Error creating bu write lock stream " << strerror(errno) << std::endl;
    }
    else
      {
	bu_readlock_fd_ = open(bulockfile.c_str(),O_RDWR);
	if(bu_readlock_fd_==-1) std::cout << "problem with creating filedesc for bureadlock " << bulockfile << " "
					  << strerror(errno) << "\n";
	else
	  std::cout << "creating filedesc for bureadlock " 
		    << bu_readlock_fd_ << "\n";
	bu_r_lock_stream = fdopen( bu_readlock_fd_,"r+");
	if(bu_r_lock_stream==0)
	  std::cout << "Error creating bu read lock stream " << strerror(errno) << std::endl;

	fu_readwritelock_fd_ = open(fulockfile.c_str(),O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
	if(fu_readwritelock_fd_==-1) std::cout << "problem with creating filedesc for fuwritelock " 
					       << strerror(errno) << "\n";
	else
	  std::cout << "creating filedesc for fureadwritelock " 
		    << fu_readwritelock_fd_ << "\n";
	fu_rw_lock_stream = fdopen( fu_readwritelock_fd_,"w");
	if(fu_rw_lock_stream==0)
	  std::cout << "Error creating fu read/write lock stream " << strerror(errno) << std::endl;

      }
    std::cout << "DaqDirector - preBeginRun success" << std::endl;
  }
 
  std::string EvFDaqDirector::getFileForLumi(unsigned int ls){
    std::string retval = bu_base_open_dir_;
    retval += "/ls";
    std::ostringstream ost;
    ost << std::setfill('0') << std::setw(6) << ls << ".raw";
    retval += ost.str();
    return retval;
  }

  std::string EvFDaqDirector::getWorkdirFileForLumi(unsigned int ls, unsigned int index){
    std::string retval = bu_base_open_dir_;
    retval += "/ls";
    std::ostringstream ost;
    ost << std::setfill('0') << std::setw(6) << ls << "_" << index << ".raw";
    retval += ost.str();
    return retval;
  }

  std::string EvFDaqDirector::getPathForFU(){
    return sm_base_dir_;
  }

  void EvFDaqDirector::removeFile(std::string &filename){
    int retval = remove(filename.c_str());
    if(retval != 0) std::cout << "Could not remove used file " << filename << " error "
			      << strerror(errno) << "\n";

  }

  void EvFDaqDirector::removeFile(unsigned int ls){
    std::string filename = getFileForLumi(ls);
    removeFile(filename);
  }

  void EvFDaqDirector::updateBuLock(unsigned int ls){
    int check=0;
    fcntl(bu_writelock_fd_,F_SETLKW,&bu_w_flk);
    if(bu_w_lock_stream != 0){
      check = fseek(bu_w_lock_stream,0,SEEK_SET);
      if(check==0)
	fprintf(bu_w_lock_stream,"%u",ls);
      else
	std::cout << "seek on bu write lock for updating failed with error " << strerror(errno) 
		<< std::endl;
    }
    else
      std::cout << "bu write lock stream is invalid " << strerror(errno) 
		<< std::endl;

    fcntl(bu_writelock_fd_,F_SETLKW,&bu_w_fulk);
  }

  int EvFDaqDirector::updateFuLock(unsigned int &ls){
    unsigned int retval = 0;
    fcntl(fu_readwritelock_fd_,F_SETLKW,&fu_rw_flk);
    if(fu_rw_lock_stream != 0){
      unsigned int readval;
      int check = 0;
      unsigned int *p = &readval;
      check = fseek(fu_rw_lock_stream,0,SEEK_SET);
      if(check==0){
	fscanf(fu_rw_lock_stream,"%u",p);
	retval = readval;
      }
      if(retval>=ls)ls=retval+1;
      check = fseek(fu_rw_lock_stream,0,SEEK_SET);
      if(check==0)
	fprintf(fu_rw_lock_stream,"%u",ls);
      else
	std::cout << "seek on fu read/write lock for updating failed with error " << strerror(errno) 
		<< std::endl;
    }
    else
      std::cout << "fu read/write lock stream is invalid " << strerror(errno) 
		<< std::endl;

    fcntl(fu_readwritelock_fd_,F_SETLKW,&fu_rw_fulk);
    return ls;
  }

  int EvFDaqDirector::readBuLock(){
    int retval=-1;
    // check if lock file has disappeared and if so whether directory is empty (signal end of run)
    if(!bulock() && dirManager_.checkDirEmpty(bu_base_dir_)) return retval;  
    if(fcntl(bu_readlock_fd_,F_SETLKW,&bu_r_flk) != 0) retval = 0;
    if(bu_r_lock_stream){
      unsigned int readval;
      int check = 0;
      unsigned int *p = &readval;
      check = fseek(bu_r_lock_stream,0,SEEK_SET);
      if(check==0){
	fscanf(bu_r_lock_stream,"%u",p);
	retval = readval;
      }
    }
    else{
      std::cout << "error reading lock file " << strerror(errno) << std::endl;
      retval = -1;
    }
    fcntl(bu_readlock_fd_,F_SETLKW,&bu_r_fulk);
    std::cout << "readbulock returning " << retval << std::endl;
    return retval;
  }


  int EvFDaqDirector::readFuLock(){
    int retval=-1;

    if(!fulock() && dirManager_.checkDirEmpty(bu_base_dir_)) return retval;  
    if(fcntl(fu_readwritelock_fd_,F_SETLKW,&fu_rw_flk) != 0) retval = 0;
    if(fu_rw_lock_stream){
      unsigned int readval;
      int check = 0;
      unsigned int *p = &readval;
      check = fseek(fu_rw_lock_stream,0,SEEK_SET);
      if(check==0){
	fscanf(fu_rw_lock_stream,"%u",p);
	retval = readval;
      }
    }
    else{
      std::cout << "error reading lock file " << strerror(errno) << std::endl;
      retval = -1;
    }
    fcntl(fu_readwritelock_fd_,F_SETLKW,&fu_rw_fulk);
    std::cout << "readfulock returning " << retval << std::endl;
    return retval;
  }


  bool EvFDaqDirector::bulock(){
    struct stat buf;
    std::string lockfile = bu_base_dir_;
    lockfile += "/bu.lock";
    bool retval = (stat(lockfile.c_str(),&buf)==0);
    if(!retval){
      close(bu_readlock_fd_);
      close(bu_writelock_fd_);
    }
    std::cout << "stat of lockfile returned " << retval << std::endl;
    return retval;
  }

  bool EvFDaqDirector::fulock(){
    struct stat buf;
    std::string lockfile = bu_base_dir_;
    lockfile += "/fu.lock";
    bool retval = (stat(lockfile.c_str(),&buf)==0);
    if(!retval){
      close(fu_readwritelock_fd_);
      close(fu_readwritelock_fd_);
    }
    std::cout << "stat of lockfile returned " << retval << std::endl;
    return retval;
  }

  // TAG BU JSON here too??
  void EvFDaqDirector::writeLsStatisticsBU(unsigned int ls, unsigned int events, unsigned long long totsize, long long lsusec){
    if(bu_w_monitor_stream != 0){
      int check = fseek(bu_w_monitor_stream,0,SEEK_SET);
      if(check==0){
	fprintf(bu_w_monitor_stream,"%u %u %llu %f %f %012lld",ls, events, totsize, double(totsize)/double(events)/1000000.,
		double(totsize)/double(lsusec), lsusec);
	fflush(bu_w_monitor_stream);
      }
      else
	std::cout << "seek on bu write monitor for updating failed with error " << strerror(errno) 
		  << std::endl;
    }
    else
      std::cout << "bu write monitor stream is invalid " << strerror(errno) 
		<< std::endl;

  }
  void EvFDaqDirector::writeDiskAndThrottleStat(double fraction,int highWater,int lowWater){
    if(bu_t_monitor_stream != 0){
      int check = fseek(bu_t_monitor_stream,0,SEEK_SET);
      if(check==0)
	fprintf(bu_t_monitor_stream,"%f %d %d",fraction,highWater,lowWater);
      else
	std::cout << "seek on disk write monitor for updating failed with error " << strerror(errno) 
		  << std::endl;
    }
    else
      std::cout << "disk write monitor stream is invalid " << strerror(errno) 
		<< std::endl;
    
  }

}
