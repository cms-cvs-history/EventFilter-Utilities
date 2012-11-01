#ifndef EVFMTRAWEVENTFILEWRITERFORBU
#define EVFMTRAWEVENTFILEWRITERFORBU

// $Id: MTRawEventFileWriterForBU.h,v 1.1.2.2 2012/09/26 22:06:27 smorovic Exp $

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "IOPool/Streamer/interface/FRDEventMessage.h"


#ifdef linux
#include <atomic>
#include <mutex>
#include <thread>
#endif

#include <fstream>
#include <deque>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "boost/shared_array.hpp"

namespace fwriter {
  class EventContainer;
}

class MTRawEventFileWriterForBU 
{
 public:

  explicit MTRawEventFileWriterForBU(edm::ParameterSet const& ps);
  ~MTRawEventFileWriterForBU();

  void doOutputEvent(FRDEventMsgView const& msg);
  void doOutputEvent(boost::shared_array<unsigned char> & msg);
  void doOutputEventFragment(unsigned char* dataPtr,
                             unsigned long dataSize);
  void doFlushFile();
  uint32 adler32(uint32 adlera, uint32 adlerb) const { return 0;/*return (adlerb_ << 16) | adlera_;*/ }

  void start() {}
  void stop() {
    finishThreads();
  }
  void initialize(std::string const& name);
  bool sharedMode() const {return sharedMode_;}
 private:

  std::string fileName_;

  inline void queueEvent(const char* buffer,unsigned long size);
  inline void queueEvent(boost::shared_array<unsigned char> & msg);
  void dispatchThreads(std::string fileBase, unsigned int instances, std::string suffix);

  void finishThreads()
  {
    close_flag_=true;
#ifdef linux
    for (auto w: writers) w->join();
    writers.clear();
#endif
  }
  //opens file and waits for events
  void threadRunner(std::string fileName, unsigned int instance);

  //std::string name_;
  unsigned int count_;
  unsigned int numWriters_;
  unsigned int eventBufferSize_;
  bool sharedMode_;
#ifdef linux
  std::atomic<bool> close_flag_;
  std::mutex queue_lock;
#endif

  //event buffer
  std::deque<unsigned int> freeIds;
  std::deque<unsigned int> queuedIds;
  std::vector<fwriter::EventContainer*> EventPool;
  //thread pool
#ifdef linux
  std::vector<std::auto_ptr<std::thread>> writers;
#endif
  std::vector<uint32> v_adlera_;
  std::vector<uint32> v_adlerb_;

  unsigned char * fileHeader_;
};
#endif
