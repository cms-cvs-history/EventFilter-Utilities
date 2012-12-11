#ifndef EVF_FASTMONITORINGTHREAD
#define EVF_FASTMONITORINGTHREAD

#include "boost/thread/thread.hpp"

#include <iostream>

namespace evf{

  class FastMonitoringService;

  class FastMonitoringThread{
  public:
    enum Macrostate { sInit = 0, sJobReady, sRunGiven, sRunning, sStopping,
		      sShuttingDown, sDone, sJobEnded, sError, sErrorEnded, sEnd, sInvalid,MCOUNT}; 
    struct MonitorData
    {
      Macrostate macrostate_;
      const void *ministate_;
      const void *microstate_;
      unsigned int eventnumber_;
      unsigned int processed_;
      unsigned int lumisection_; //only updated on beginLumi signal
      unsigned int prescaleindex_; // ditto
    };
    // a copy of the Framework/EventProcessor states 


    FastMonitoringThread() : m_stoprequest(false){}
      
    void start(void (FastMonitoringService::*fp)(),FastMonitoringService *cp){
      assert(!m_thread);
      m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(fp,cp)));
    }
    void stop(){
      assert(m_thread);
      m_stoprequest=true;
      m_thread->join();
    }

  private:

    volatile bool m_stoprequest;
    boost::shared_ptr<boost::thread> m_thread;
    MonitorData m_data;
    boost::mutex lock_;

    friend class FastMonitoringService;
  };
} //end namespace evf
#endif
