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
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"


FedRawDataInputSource::FedRawDataInputSource(edm::ParameterSet const& pset, edm::InputSourceDescription const& desc) :
edm::RawInputSource(pset,desc),
runDirectory_(pset.getUntrackedParameter<std::string>("runDirectory")),
fileIndex_(0)
{
  std::ostringstream myDir;
  myDir << std::setfill('0') << std::setw(5) << getpid();
  workingDirectory_ = runDirectory_;
  workingDirectory_ /= myDir.str();
  boost::filesystem::create_directories(workingDirectory_);
  produces<FEDRawDataCollection>();
}


FedRawDataInputSource::~FedRawDataInputSource()
{}


std::auto_ptr<edm::Event>
FedRawDataInputSource::readOneEvent()
{
  uint32_t version, runNumber, lumiSection, eventNumber;

  if ( !finput_.is_open() ||
    (finput_.peek() == EOF) ) openNextFile();
  if ( !finput_.is_open() )
  {
    // run has ended
    return std::auto_ptr<edm::Event>(0);
  }
  finput_.read((char*)&version, sizeof(uint32_t));
  assert( version == 2 );
  finput_.read((char*)&runNumber, sizeof(uint32_t));
  finput_.read((char*)&lumiSection, sizeof(uint32_t));
  finput_.read((char*)&eventNumber, sizeof(uint32_t));

  std::auto_ptr<FEDRawDataCollection> rawData(new FEDRawDataCollection);
  edm::Timestamp tstamp = fillFEDRawDataCollection(rawData);

  std::auto_ptr<edm::Event> event = makeEvent(runNumber, lumiSection, eventNumber, tstamp);
  
  event->put(rawData);
 
  return event;
}


edm::Timestamp
FedRawDataInputSource::fillFEDRawDataCollection(std::auto_ptr<FEDRawDataCollection>& rawData)
{
  edm::Timestamp tstamp;
  size_t totalEventSize = 0;
  uint32_t fedSizes[1024];
  finput_.read((char*)fedSizes, 1024*sizeof(uint32_t));
  for (unsigned int i=0;i<1024;i++) {
    totalEventSize += fedSizes[i];
  }
  
  unsigned int gtpevmsize = fedSizes[FEDNumbering::MINTriggerGTPFEDID];
  if ( gtpevmsize > 0 )
    evf::evtn::evm_board_setformat(gtpevmsize);
  
  char* event = new char[totalEventSize];
  finput_.read(event, totalEventSize);
  
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


void
FedRawDataInputSource::openNextFile()
{
  finput_.close();
  finput_.clear();

  boost::filesystem::path nextFile = workingDirectory_;
  std::ostringstream fileName;
  fileName << std::setfill('0') << std::setw(16) << fileIndex_++ << ".raw";
  nextFile /= fileName.str();
  finput_.open(nextFile.c_str(), std::ios::in|std::ios::binary);
  
  if ( ! finput_.is_open() ) grabNextFile(nextFile);
}


void
FedRawDataInputSource::grabNextFile(boost::filesystem::path const& nextFile)
{
  std::vector<boost::filesystem::path> files;
  
  try
  {
    do
    {
      boost::filesystem::directory_iterator itEnd;
      for ( boost::filesystem::directory_iterator it(runDirectory_);
            it != itEnd; ++it)
      {
        if ( it->path().extension() == ".raw" )
          files.push_back(*it);
      }
    }
    while ( files.empty() && !runEnded() );

    if ( !files.empty() )
    {
      std::sort(files.begin(), files.end());
      boost::filesystem::rename(files.front(), nextFile);
      finput_.open(nextFile.c_str(), std::ios::in|std::ios::binary);
    }
  }
  catch (const boost::filesystem::filesystem_error& ex)
  {
    std::cout << ex.what() << std::endl;
  }
}


bool
FedRawDataInputSource::runEnded() const
{
  boost::filesystem::path endOfRunMarker = runDirectory_;
  endOfRunMarker /= "EndOfRunMarker";
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
