#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "DataFormats/FEDRawData/interface/FEDNumbering.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"

#include "DataFormats/Provenance/interface/LuminosityBlockAuxiliary.h"
#include "DataFormats/Provenance/interface/EventAuxiliary.h"
#include "DataFormats/Provenance/interface/EventID.h"

#include "EventFilter/FEDInterface/interface/GlobalEventNumber.h"
#include "EventFilter/Utilities/plugins/FedRawDataInputSource.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/InputSourceDescription.h"
#include "FWCore/Framework/interface/InputSourceMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "IOPool/Streamer/interface/FRDEventMessage.h"

#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"


FedRawDataInputSource::FedRawDataInputSource(edm::ParameterSet const& pset, edm::InputSourceDescription const& desc) :
edm::RawInputSource(pset,desc),
daqProvenanceHelper_(edm::TypeID(typeid(FEDRawDataCollection))),
formatVersion_(0),
fileIndex_(0),
fileStream_(0),
workDirCreated_(false),
eventID_()
{
  findRunDir( pset.getUntrackedParameter<std::string>("rootDirectory") );
  daqProvenanceHelper_.daqInit(productRegistryUpdate());
  setNewRun();
  setRunAuxiliary(new edm::RunAuxiliary(runNumber_, edm::Timestamp::beginOfTime(), edm::Timestamp::invalidTimestamp()));
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
    usleep(500000);
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

  std::string rnString = runDirectory_.string().substr(runDirectory_.string().find("run")+3);
  runNumber_ = atoi(rnString.c_str());

  edm::LogInfo("FedRawDataInputSource") << "Getting data from " << runDirectory_.string();

}

bool FedRawDataInputSource::checkNextEvent()
{
  if ( eofReached() && !openNextFile() )
  {
    // run has ended
    if (workDirCreated_)
      boost::filesystem::remove(workingDirectory_);
    return false;
  }

  FRDEventHeader_V2 eventHeader;
  fread((void*)&eventHeader, sizeof(uint32_t), 4, fileStream_);
  assert( eventHeader.version_ > 1);
  formatVersion_ = eventHeader.version_;

  if(!luminosityBlockAuxiliary() || luminosityBlockAuxiliary()->luminosityBlock() != eventHeader.lumi_)
  {
    resetLuminosityBlockAuxiliary();
    timeval tv;
    gettimeofday(&tv,0);
    edm::Timestamp lsopentime((unsigned long long)tv.tv_sec*1000000+(unsigned long long)tv.tv_usec);
    edm::LuminosityBlockAuxiliary* luminosityBlockAuxiliary =
	            new edm::LuminosityBlockAuxiliary(runAuxiliary()->run(),eventHeader.lumi_,
				    lsopentime, edm::Timestamp::invalidTimestamp());
    setLuminosityBlockAuxiliary(luminosityBlockAuxiliary);
  }
  eventID_ = edm::EventID(eventHeader.run_, eventHeader.lumi_, eventHeader.event_);
  setEventCached();
  return true;

}


edm::EventPrincipal*
FedRawDataInputSource::read(edm::EventPrincipal& eventPrincipal)
{
  std::auto_ptr<FEDRawDataCollection> rawData(new FEDRawDataCollection);
  edm::Timestamp tstamp = fillFEDRawDataCollection(rawData);

  edm::EventAuxiliary aux(eventID_, processGUID(), tstamp, true, edm::EventAuxiliary::PhysicsTrigger);

  edm::EventPrincipal * e = makeEvent(eventPrincipal, aux);

  edm::WrapperOwningHolder edp(new edm::Wrapper<FEDRawDataCollection>(rawData), edm::Wrapper<FEDRawDataCollection>::getInterface());
  e->put(daqProvenanceHelper_.constBranchDescription_, edp, daqProvenanceHelper_.dummyProvenance_);
  return e;
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
  uint32_t eventSize = 0;
  uint32_t paddingSize = 0;
  if ( formatVersion_ >= 3 )
  {
    fread((void*)&eventSize, sizeof(uint32_t), 1, fileStream_);
    fread((void*)&paddingSize, sizeof(uint32_t), 1, fileStream_);
  }

  uint32_t fedSizes[1024];
  fread((void*)fedSizes, sizeof(uint32_t), 1024, fileStream_);
  if ( formatVersion_ < 3 )
  {
    for (unsigned int i=0;i<1024;i++)
      eventSize += fedSizes[i];
  }
  
  unsigned int gtpevmsize = fedSizes[FEDNumbering::MINTriggerGTPFEDID];
  if ( gtpevmsize > 0 )
    evf::evtn::evm_board_setformat(gtpevmsize);
  
  char* event = new char[eventSize];
  fread((void*)event, eventSize, 1, fileStream_);
  
  while ( eventSize>0 )
  {
    eventSize -= sizeof(fedt_t);
    const fedt_t* fedTrailer = (fedt_t*)(event+eventSize);
    const uint32_t fedSize = FED_EVSZ_EXTRACT(fedTrailer->eventsize) << 3; //trailer length counts in 8 bytes
    eventSize -= (fedSize - sizeof(fedh_t));
    const fedh_t* fedHeader = (fedh_t *)(event+eventSize);
    const uint16_t fedId = FED_SOID_EXTRACT(fedHeader->sourceid);
    if ( fedId == FEDNumbering::MINTriggerGTPFEDID )
    {
      const uint64_t gpsl = evf::evtn::getgpslow((unsigned char*)fedHeader);
      const uint64_t gpsh = evf::evtn::getgpshigh((unsigned char*)fedHeader);
      tstamp = edm::Timestamp(static_cast<edm::TimeValue_t>((gpsh << 32) + gpsl));
    }
    FEDRawData& fedData=rawData->FEDData(fedId);
    fedData.resize(fedSize);
    memcpy(fedData.data(),event+eventSize,fedSize);
  }
  assert( eventSize == 0 );
  fseek(fileStream_, paddingSize, SEEK_CUR);
  delete event;
  return tstamp;
}


bool
FedRawDataInputSource::openNextFile()
{
  if (!workDirCreated_) createWorkingDirectory();
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
  std::cout << " tried to open file.. " <<  nextFile << " fd:" << fileDescriptor << std::endl;
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


void
FedRawDataInputSource::preForkReleaseResources()
{
}

void
FedRawDataInputSource::postForkReacquireResources(boost::shared_ptr<edm::multicore::MessageReceiverForSource>)
{
  createWorkingDirectory();
  InputSource::rewind();
  setRunAuxiliary(new edm::RunAuxiliary(runNumber_, edm::Timestamp::beginOfTime(), edm::Timestamp::invalidTimestamp()));
}

void
FedRawDataInputSource::rewind_() {}

void FedRawDataInputSource::createWorkingDirectory() {
 
  char thishost[256];
  gethostname(thishost,255);
  std::ostringstream myDir;
  myDir << std::setfill('0') << std::setw(5) << thishost << "_" << getpid();
  workingDirectory_ = runDirectory_;
  workingDirectory_ /= myDir.str();
  boost::filesystem::create_directories(workingDirectory_);
  workDirCreated_=true;
}

// define this class as an input source
DEFINE_FWK_INPUT_SOURCE(FedRawDataInputSource);


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
