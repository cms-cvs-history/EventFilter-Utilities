// $Id: RawEventFileWriterForBU.cc,v 1.1.2.2 2012/11/28 18:18:42 smorovic Exp $

#include "RawEventFileWriterForBU.h"
#include "FWCore/Utilities/interface/Adler32Calculator.h"
#include "FWCore/Utilities/interface/Exception.h"

#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <string.h>

RawEventFileWriterForBU::RawEventFileWriterForBU(edm::ParameterSet const& ps): outfd_(0)
{
  //  initialize(ps.getUntrackedParameter<std::string>("fileName", "testFRDfile.dat"));
}

RawEventFileWriterForBU::RawEventFileWriterForBU(std::string const& fileName)
{
  //  initialize(fileName);
}

RawEventFileWriterForBU::~RawEventFileWriterForBU()
{
  //  ost_->close();
}

void RawEventFileWriterForBU::doOutputEvent(FRDEventMsgView const& msg)
{
  //  ost_->write((const char*) msg.startAddress(), msg.size());
  //  if (ost_->fail()) {
  ssize_t retval =  write(outfd_,(void*)msg.startAddress(), msg.size());
  if(retval!= msg.size()){
    throw cms::Exception("RawEventFileWriterForBU", "doOutputEvent")
      << "Error writing FED Raw Data event data to "
      << fileName_ << ".  Possibly the output disk "
      << "is full?" << std::endl;
  }

//   ost_->flush();
//   if (ost_->fail()) {
//     throw cms::Exception("RawEventFileWriterForBU", "doOutputEvent")
//       << "Error writing FED Raw Data event data to "
//       << fileName_ << ".  Possibly the output disk "
//       << "is full?" << std::endl;
//   }
  
  //  cms::Adler32((const char*) msg.startAddress(), msg.size(), adlera_, adlerb_);
}

void RawEventFileWriterForBU::doFlushFile()
{
  ost_->flush();
  if (ost_->fail()) {
    throw cms::Exception("RawEventFileWriterForBU", "doOutputEvent")
      << "Error writing FED Raw Data event data to "
      << fileName_ << ".  Possibly the output disk "
      << "is full?" << std::endl;
  }
}

void RawEventFileWriterForBU::doOutputEventFragment(unsigned char* dataPtr,
                                               unsigned long dataSize)
{
  ost_->write((const char*) dataPtr, dataSize);
  if (ost_->fail()) {
    throw cms::Exception("RawEventFileWriterForBU", "doOutputEventFragment")
      << "Error writing FED Raw Data event data to "
      << fileName_ << ".  Possibly the output disk "
      << "is full?" << std::endl;
  }

  ost_->flush();
  if (ost_->fail()) {
    throw cms::Exception("RawEventFileWriterForBU", "doOutputEventFragment")
      << "Error writing FED Raw Data event data to "
      << fileName_ << ".  Possibly the output disk "
      << "is full?" << std::endl;
  }

  cms::Adler32((const char*) dataPtr, dataSize, adlera_, adlerb_);
}

void RawEventFileWriterForBU::initialize(std::string const& destinationDir, std::string const& name, int ls)
{
  std::string oldFileName = fileName_;
  fileName_ = name;
  destinationDir_ = destinationDir;
  if(outfd_!=0){
    close(outfd_);
    outfd_=0;
  }
  outfd_ = open(fileName_.c_str(), O_WRONLY | O_CREAT,  S_IRWXU);
  if(outfd_ == -1) {
    throw cms::Exception("RawEventFileWriterForBU","initialize")
      << "Error opening FED Raw Data event output file: " << name 
      << ": " << strerror(errno) << "\n";
  }
  ost_.reset(new std::ofstream(name.c_str(), std::ios_base::binary | std::ios_base::out));
 
  //move old file to done directory 
  if (!oldFileName.empty()) {
	  //rename(oldFileName.c_str(),destinationDir_.c_str());
	  int fretval = rename(oldFileName.c_str(),(destinationDir_+oldFileName.substr(oldFileName.rfind("/"))).c_str());
	  // if (debug_)
	  std::cout << " tried move " << oldFileName << " to " << destinationDir_
			  << " status "  << fretval << " errno " << strerror(errno) << std::endl;
  }


  if (!ost_->is_open()) {
    throw cms::Exception("RawEventFileWriterForBU","initialize")
      << "Error opening FED Raw Data event output file: " << name << "\n";
  }

  adlera_ = 1;
  adlerb_ = 0;
}

void RawEventFileWriterForBU::endOfLS(int ls)
{
}

