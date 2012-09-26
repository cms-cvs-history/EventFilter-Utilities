#ifndef EVFMTRAWEVENTFILEWRITERFORBU
#define EVFMTRAWEVENTFILEWRITERFORBU

// $Id: MTRawEventFileWriterForBU.h,v 1.1.2.1 2012/09/06 20:21:28 smorovic Exp $

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "IOPool/Streamer/interface/FRDEventMessage.h"

#include <fstream>
#include <atomic>
#include <mutex>
#include <deque>
#include <thread>

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
  //explicit MTRawEventFileWriterForBU(std::string const& fileName);
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
    for (auto w: writers) w->join();
    writers.clear();
  }
  //opens file and waits for events
  void threadRunner(std::string fileName, unsigned int instance);

  //std::string name_;
  unsigned int count_;
  unsigned int numWriters_;
  unsigned int eventBufferSize_;
  bool sharedMode_;
  std::atomic<bool> close_flag_;
  std::mutex queue_lock;

  //event buffer
  std::deque<unsigned int> freeIds;
  std::deque<unsigned int> queuedIds;
  std::vector<fwriter::EventContainer*> EventPool;
  //thread pool
  std::vector<std::auto_ptr<std::thread>> writers;
  std::vector<uint32> v_adlera_;
  std::vector<uint32> v_adlerb_;

  unsigned char * fileHeader_;
};
#endif
