#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "DataFormats/FEDRawData/interface/FEDNumbering.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "EventFilter/FEDInterface/interface/GlobalEventNumber.h"
#include "EventFilter/Utilities/plugins/FedRawDataInputSource.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/InputSourceDescription.h"
#include "FWCore/Framework/interface/InputSourceMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"


FedRawDataInputSource::FedRawDataInputSource(edm::ParameterSet const& pset, edm::InputSourceDescription const& desc) :
edm::RawInputSource(pset,desc),
fileIndex_(0),
fileStream_(0)
{
  findRunDir( pset.getUntrackedParameter<std::string>("rootDirectory") );

  std::ostringstream myDir;
  myDir << std::setfill('0') << std::setw(5) << getpid();
  workingDirectory_ = runDirectory_;
  workingDirectory_ /= myDir.str();
  boost::filesystem::create_directories(workingDirectory_);
  produces<FEDRawDataCollection>();
}


FedRawDataInputSource::~FedRawDataInputSource()
{
  if (fileStream_) fclose(fileStream_);
  fileStream_ = 0;
}


void
FedRawDataInputSource::findRunDir(const std::string& rootDirectory)
{
  // The run dir should be set via the configuration
  // For now, just grab the latest run directory available
  
  std::vector<boost::filesystem::path> dirs;
  boost::filesystem::directory_iterator itEnd;
  do {
    for ( boost::filesystem::directory_iterator it(rootDirectory);
          it != itEnd; ++it)
    {
      if ( boost::filesystem::is_directory(it->path()) &&
        std::string::npos != it->path().string().find("run") )
        dirs.push_back(*it);
    }
  } while ( dirs.empty() );
  std::sort(dirs.begin(), dirs.end());
  runDirectory_ = dirs.back();
  edm::LogInfo("FedRawDataInputSource") << "Getting data from " << runDirectory_.string();
}


std::auto_ptr<edm::Event>
FedRawDataInputSource::readOneEvent()
{
  struct
  {
    uint32_t version;
    uint32_t runNumber;
    uint32_t lumiSection;
    uint32_t eventNumber;
  } eventHeader;

  if ( eofReached() && !openNextFile() )
  {
    // run has ended
    boost::filesystem::remove(workingDirectory_);
    return std::auto_ptr<edm::Event>(0);
  }
  fread((void*)&eventHeader, sizeof(uint32_t), 4, fileStream_);
  assert( eventHeader.version == 2 );

  std::auto_ptr<FEDRawDataCollection> rawData(new FEDRawDataCollection);
  edm::Timestamp tstamp = fillFEDRawDataCollection(rawData);

  std::auto_ptr<edm::Event> event =
    makeEvent(eventHeader.runNumber, eventHeader.lumiSection, eventHeader.eventNumber, tstamp);
  
  event->put(rawData);
 
  return event;
}


bool
FedRawDataInputSource::eofReached() const
{
  if ( fileStream_ == 0 ) return true;

  int c;
  c = fgetc(fileStream_);
  ungetc(c, fileStream_);

  return (c == EOF);
}


edm::Timestamp
FedRawDataInputSource::fillFEDRawDataCollection(std::auto_ptr<FEDRawDataCollection>& rawData)
{
  edm::Timestamp tstamp;
  size_t totalEventSize = 0;
  uint32_t fedSizes[1024];
  fread((void*)fedSizes, sizeof(uint32_t), 1024, fileStream_);
  for (unsigned int i=0;i<1024;i++) {
    totalEventSize += fedSizes[i];
  }
  
  unsigned int gtpevmsize = fedSizes[FEDNumbering::MINTriggerGTPFEDID];
  if ( gtpevmsize > 0 )
    evf::evtn::evm_board_setformat(gtpevmsize);
  
  char* event = new char[totalEventSize];
  fread((void*)event, totalEventSize, 1, fileStream_);
  
  while ( totalEventSize>0 )
  {
    totalEventSize -= sizeof(fedt_t);
    const fedt_t* fedTrailer = (fedt_t*)(event+totalEventSize);
    const uint32_t fedSize = FED_EVSZ_EXTRACT(fedTrailer->eventsize) << 3; //trailer length counts in 8 bytes
    totalEventSize -= (fedSize - sizeof(fedh_t));
    const fedh_t* fedHeader = (fedh_t *)(event+totalEventSize);
    const uint16_t fedId = FED_SOID_EXTRACT(fedHeader->sourceid);
    if ( fedId == FEDNumbering::MINTriggerGTPFEDID )
    {
      const uint64_t gpsl = evf::evtn::getgpslow((unsigned char*)fedHeader);
      const uint64_t gpsh = evf::evtn::getgpshigh((unsigned char*)fedHeader);
      tstamp = edm::Timestamp(static_cast<edm::TimeValue_t>((gpsh << 32) + gpsl));
    }
    FEDRawData& fedData=rawData->FEDData(fedId);
    fedData.resize(fedSize);
    memcpy(fedData.data(),event+totalEventSize,fedSize);
  }
  return tstamp;
}


bool
FedRawDataInputSource::openNextFile()
{
  boost::filesystem::path nextFile = workingDirectory_;
  std::ostringstream fileName;
  fileName << std::setfill('0') << std::setw(16) << fileIndex_++ << ".raw";
  nextFile /= fileName.str();

  openFile(nextFile);
  
  while ( !grabNextFile(nextFile) ) ::usleep(1000);

  return ( fileStream_ != 0 );
}


void
FedRawDataInputSource::openFile(boost::filesystem::path const& nextFile)
{
  if (fileStream_)
  {
    fclose(fileStream_);
    fileStream_ = 0;
    boost::filesystem::remove(openFile_); // wont work in case of forked children
  }
  
  const int fileDescriptor = open(nextFile.c_str(), O_RDONLY);
  if ( fileDescriptor != -1 )
  {
    fileStream_ = fdopen(fileDescriptor, "rb");
    openFile_ = nextFile;
  }
}


bool
FedRawDataInputSource::grabNextFile(boost::filesystem::path const& nextFile)
{
  std::vector<boost::filesystem::path> files;
  
  try
  {
    boost::filesystem::directory_iterator itEnd;
    for ( boost::filesystem::directory_iterator it(runDirectory_);
          it != itEnd; ++it)
    {
      if ( it->path().extension() == ".raw" )
        files.push_back(*it);
    }
    
    if ( files.empty() )
    {
      if ( runEnded() ) return true;
    }
    else
    {
      std::sort(files.begin(), files.end());
      boost::filesystem::rename(files.front(), nextFile);
      openFile(nextFile);
      return true;
    }
  }
  catch (const boost::filesystem::filesystem_error& ex)
  {
    // Another process grabbed the file.
  }
  return false;
}


bool
FedRawDataInputSource::runEnded() const
{
  boost::filesystem::path endOfRunMarker = runDirectory_;
  endOfRunMarker /= "EndOfRun.jsn";
  return boost::filesystem::exists(endOfRunMarker);
}


// define this class as an input source
DEFINE_FWK_INPUT_SOURCE(FedRawDataInputSource);


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
