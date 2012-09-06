// $Id: RawEventFileWriterForBU.cc,v 1.1.2.1 2012/09/04 12:49:24 meschi Exp $

#include "MTRawEventFileWriterForBU.h"
#include "FWCore/Utilities/interface/Adler32Calculator.h"
#include "FWCore/Utilities/interface/Exception.h"

#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <boost/tokenizer.hpp>

namespace fwriter {
  class EventContainer {
    public:
      EventContainer(unsigned int evtBufSize)
      {
	writtenSize_=0;
	evtBufSize_=evtBufSize;
	data_.reset(new unsigned char [evtBufSize]);
      }
      ~EventContainer() {}

      unsigned int getSize() const {return writtenSize_;}
      unsigned int getBufSize() const {return evtBufSize_;}
      unsigned char* getBuffer() const {return data_.get();}

      void putNewEvent(unsigned char* addr, unsigned int size) {
	if (size>evtBufSize_) {
	  data_.reset(new unsigned char[size]);
	  evtBufSize_=size;
	}
	memcpy(data_.get(),addr,size);
	writtenSize_=size;
      }

    private:
      unsigned int writtenSize_;
      unsigned int evtBufSize_;
      std::auto_ptr<unsigned char> data_;
  };
}

MTRawEventFileWriterForBU::MTRawEventFileWriterForBU(edm::ParameterSet const& ps):
  numWriters_(ps.getUntrackedParameter<unsigned int>("numWriters",1))
  ,eventBufferSize_(ps.getUntrackedParameter<unsigned int>("eventBufferSize",30))
{
  for (unsigned int i=0;i<eventBufferSize_;i++) {
    EventPool.push_back(new fwriter::EventContainer(1048576));
    freeIds.push_back(i);
  }
  //  initialize(ps.getUntrackedParameter<std::string>("fileName", "testFRDfile.dat"));
}

//MTRawEventFileWriterForBU::MTRawEventFileWriterForBU(std::string const& fileName,unsigned int numWriters)
//{
    //  initialize(fileName);
    //numWriters_=numWriters;
//}

MTRawEventFileWriterForBU::~MTRawEventFileWriterForBU()
{
  finishThreads();
}

void MTRawEventFileWriterForBU::doOutputEvent(FRDEventMsgView const& msg)
{
  queueEvent((const char*)msg.startAddress(), msg.size());
}

void MTRawEventFileWriterForBU::doFlushFile()
{
  //not implemented
}

void MTRawEventFileWriterForBU::doOutputEventFragment(unsigned char* dataPtr,
                                               unsigned long dataSize)
{
  queueEvent((const char*) dataPtr, dataSize);

  /*
  ost_->flush();
  if (ost_->fail()) {
    throw cms::Exception("RawEventFileWriterForBU", "doOutputEventFragment")
      << "Error writing FED Raw Data event data to "
      << fileName_ << ".  Possibly the output disk "
      << "is full?" << std::endl;
  }
  */

  //cms::Adler32((const char*) dataPtr, dataSize, adlera_, adlerb_);
}

void MTRawEventFileWriterForBU::initialize(std::string const& name)
{

  std::string fileBase=name;
  std::string fileSuffix;

  boost::char_separator<char> sep(".");
  boost::tokenizer<boost::char_separator<char>> tokens(name, sep);

  fileBase=*tokens.begin();
  for (auto tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
    fileSuffix=*tok_iter;
  }
  finishThreads();
  dispatchThreads(fileBase,numWriters_,fileSuffix);
}

void MTRawEventFileWriterForBU::queueEvent(const char* buffer,unsigned long size)
{

  bool queuing = false;
  unsigned int freeId = 0xffff;
  while (!queuing) {
    queue_lock.lock();
    if (freeIds.size()) {
      freeId = freeIds.front();
      freeIds.pop_front();
      queuing = true;
    }
    queue_lock.unlock();
    if (!queuing) usleep(100000);
  }
  assert(freeId!=0xff);
  EventPool[freeId]->putNewEvent((unsigned char*)buffer,size);

  queue_lock.lock();
  queuedIds.push_back(freeId);
  queue_lock.unlock();
}

void MTRawEventFileWriterForBU::dispatchThreads(std::string fileBase, unsigned int instances, std::string suffix)
{
  close_flag_=false;
  //v_adlera_(numWriters_,1);
  //v_adlerb_(numWriters_,0);
  for (unsigned int i=0;i<instances;i++)
  {
    std::ostringstream instanceName;
    instanceName << fileBase << "_" << i;
    if (suffix.size())
      instanceName << "." << suffix;

    writers.push_back(std::auto_ptr<std::thread>(new std::thread(&MTRawEventFileWriterForBU::threadRunner,this,instanceName.str(),i)));
  }
}


void MTRawEventFileWriterForBU::threadRunner(std::string fileName,unsigned int instance)
{
  //new file..
  int outfd_ = open(fileName.c_str(), O_WRONLY | O_CREAT,  S_IRWXU);
  if(outfd_ == -1) {
    throw cms::Exception("RawEventFileWriterForBU","initialize")
      << "Error opening FED Raw Data event output file: " << fileName 
      << ": " << strerror(errno) << "\n";
  }
  std::auto_ptr<std::ofstream> ost_;
  ost_.reset(new std::ofstream(fileName.c_str(), std::ios_base::binary | std::ios_base::out));

  if (!ost_->is_open()) {
    throw cms::Exception("RawEventFileWriterForBU","initialize")
      << "Error opening FED Raw Data event output file: " << fileName << "\n";
  }

  //event writing loop
  while (1) {
    queue_lock.lock();

    if (!queuedIds.size()) {
      if (!close_flag_) {
	queue_lock.unlock();
	usleep(100000);//todo:use timed cond wait
	continue;
      }
    }
    if (close_flag_) {
      queue_lock.unlock();
      break;
    }
    //take next event
    unsigned int qid = queuedIds.back();
    queuedIds.pop_back();
    queue_lock.unlock();

    ost_->write((const char*) EventPool[qid]->getBuffer(), EventPool[qid]->getSize());
    if (ost_->fail()) {
      //todo:signal to main thread
      throw cms::Exception("RawEventFileWriterForBU", "doOutputEventFragment")
	<< "Error writing FED Raw Data event data to "
	<< fileName_ << ".  Possibly the output disk "
        << "is full?" << std::endl;
    }

    queue_lock.lock();
    freeIds.push_back(qid);
    queue_lock.unlock();
  }

  //flush and close file
  ost_->flush();
  if (ost_->fail()) {
    throw cms::Exception("RawEventFileWriterForBU", "doOutputEventFragment")
      << "Error writing FED Raw Data event data to "
      << fileName_ << ".  Possibly the output disk "
      << "is full?" << std::endl;
  }
  ost_.reset();
  if(outfd_!=0){ close(outfd_); outfd_=0;}
}
