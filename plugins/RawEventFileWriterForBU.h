#ifndef EVFRAWEVENTFILEWRITERFORBU
#define EVFRAWEVENTFILEWRITERFORBU

// $Id: RawEventFileWriterForBU.h,v 1.3 2010/02/18 09:19:02 mommsen Exp $

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "IOPool/Streamer/interface/FRDEventMessage.h"

#include <fstream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

class RawEventFileWriterForBU 
{
 public:

  explicit RawEventFileWriterForBU(edm::ParameterSet const& ps);
  explicit RawEventFileWriterForBU(std::string const& fileName);
  ~RawEventFileWriterForBU();

  void doOutputEvent(FRDEventMsgView const& msg);
  void doOutputEventFragment(unsigned char* dataPtr,
                             unsigned long dataSize);
  void doFlushFile();
  uint32 adler32() const { return (adlerb_ << 16) | adlera_; }

  void start() {}
  void stop() {}
  void initialize(std::string const& name);
 private:



  std::auto_ptr<std::ofstream> ost_;
  int outfd_;
  std::string fileName_;

  uint32 adlera_;
  uint32 adlerb_;

};
#endif
