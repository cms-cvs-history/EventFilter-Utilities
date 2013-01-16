#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <boost/algorithm/string.hpp>

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

#include "interface/shared/fed_header.h"
#include "interface/shared/fed_trailer.h"

#include "../interface/FileIO.h"
#include "../interface/JSONFileCollector.h"


FedRawDataInputSource::FedRawDataInputSource(edm::ParameterSet const& pset, edm::InputSourceDescription const& desc) :
edm::RawInputSource(pset,desc),
daqProvenanceHelper_(edm::TypeID(typeid(FEDRawDataCollection))),
fileIndex_(0),
fileStream_(0),
workDirCreated_(false),
eventID_(),
lastOpenedLumi_(0)
{
  findRunDir( pset.getUntrackedParameter<std::string>("rootDirectory") );
  daqProvenanceHelper_.daqInit(productRegistryUpdate());
  setNewRun();
  setRunAuxiliary(new edm::RunAuxiliary(runNumber_, edm::Timestamp::beginOfTime(), edm::Timestamp::invalidTimestamp()));

  count_ = 0;
  // set names of the variables to be matched with JSON Definition
  count_.setName("NEvents");

  // create a vector of all monitorable parameters to be passed to the monitor
  vector<JsonMonitorable*> monParams;
  monParams.push_back(&count_);

  // create a DataPointMonitor using vector of monitorable parameters and a path to a JSON Definition file
  mon_ = new DataPointMonitor(monParams, "/home/aspataru/cmssw/CMSSW_6_1_0_pre4/src/EventFilter/Utilities/plugins/budef.jsd");

}


FedRawDataInputSource::~FedRawDataInputSource()
{
  if (fileStream_) fclose(fileStream_);
  fileStream_ = 0;
  if (mon_ != 0)
	  delete mon_;
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
      std::cout << " it " << it->path().string() << std::endl;
      if ( boost::filesystem::is_directory(it->path()) &&
        std::string::npos != it->path().string().find("run") )
        dirs.push_back(*it);
    }
    if (dirs.empty()) usleep(500000);
  } while ( dirs.empty() );
  std::sort(dirs.begin(), dirs.end());
  runDirectory_ = dirs.back();
  runBaseDirectory_ = dirs.back();

  std::string rnString = runDirectory_.string().substr(runDirectory_.string().find("run")+3);
  runNumber_ = atoi(rnString.c_str());

  //find "bu" subdir and take files from there, if present
  for ( boost::filesystem::directory_iterator it(runDirectory_);it != itEnd; ++it)
  {
    if ( boost::filesystem::is_directory(it->path()) && boost::algorithm::ends_with(it->path().string(),"/bu"))
    { 
      runDirectory_=*it;
      break;
    }
  }

  edm::LogInfo("FedRawDataInputSource") << "Getting data from " << runDirectory_.string();

}

bool FedRawDataInputSource::checkNextEvent()
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
    resetLuminosityBlockAuxiliary();//todo:way to notify of run end?
    if (workDirCreated_)
      boost::filesystem::remove(workingDirectory_);
    return false;
  }

  //same lumi, or new lumi detected in file (old mode)
  if (!newLumiSection_)
  {
    fread((void*)&eventHeader, sizeof(uint32_t), 4, fileStream_);
    assert( eventHeader.version == 2 );

    //get new lumi from file header
    if(!luminosityBlockAuxiliary() || luminosityBlockAuxiliary()->luminosityBlock() != eventHeader.lumiSection)
    {
      resetLuminosityBlockAuxiliary();
      timeval tv;
      gettimeofday(&tv,0);
      edm::Timestamp lsopentime((unsigned long long)tv.tv_sec*1000000+(unsigned long long)tv.tv_usec);
      edm::LuminosityBlockAuxiliary* luminosityBlockAuxiliary =
	new edm::LuminosityBlockAuxiliary(runAuxiliary()->run(),eventHeader.lumiSection,
	    lsopentime, edm::Timestamp::invalidTimestamp());
      setLuminosityBlockAuxiliary(luminosityBlockAuxiliary);
    }
    eventID_ = edm::EventID(eventHeader.runNumber, eventHeader.lumiSection, eventHeader.eventNumber);
    lastOpenedLumi_=eventHeader.lumiSection;
    setEventCached();
  }
  else
  {
    //new lumi from directory name
	  resetLuminosityBlockAuxiliary();

	  //count_ = 0;

    timeval tv;
    gettimeofday(&tv,0);
    edm::Timestamp lsopentime((unsigned long long)tv.tv_sec*1000000+(unsigned long long)tv.tv_usec);
    edm::LuminosityBlockAuxiliary* luminosityBlockAuxiliary = 
      new edm::LuminosityBlockAuxiliary(runAuxiliary()->run(), lastOpenedLumi_,
	  lsopentime, edm::Timestamp::invalidTimestamp());
    setLuminosityBlockAuxiliary(luminosityBlockAuxiliary);
    //newLumiSection_=false;
  }
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
  count_.value()++;
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

  // write previous json
  boost::filesystem::path jsonFile = workingDirectory_;
  std::ostringstream fileNameJson;
  fileNameJson << std::setfill('0') << std::setw(16) << fileIndex_ << ".json";
  jsonFile /= fileNameJson.str();

  // MARK! JSON is not generated in InputSource!
  // create a DataPoint object and take a snapshot of the monitored data into it
  /*
  DataPoint dp;
  mon_->snap(dp);

  // serialize the DataPoint and output it
  string output;
  JSONSerializer::serialize(&dp, output);

  string path = jsonFile.string();
  std::cout << "CURRENT DATA DIR: " << path << std::endl;
  FileIO::writeStringToFile(path, output);
	*/
  count_ = 0;

  openFile(nextFile);//closes previous file
  searchForNextFile(nextFile);

  return ( fileStream_ != 0 || newLumiSection_ );
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
FedRawDataInputSource::searchForNextFile(boost::filesystem::path const& nextFile)
{
  newLumiSection_=false;
  do
  {
    //try to open file from same directory (same lumi?)
    if (lastOpenedLumi_>0 && !currentDataDir_.string().empty())
      if (grabNextFile(currentDataDir_,nextFile)) return true;

    //no file, check for new lumi dir
    boost::filesystem::directory_iterator itEnd;
    std::vector<boost::filesystem::path> lsdirs;
    for ( boost::filesystem::directory_iterator it(runDirectory_);it != itEnd; ++it)
    {
      std::string lsdir = it->path().string();
      size_t pos = lsdir.find("ls");
      if (boost::filesystem::is_directory(it->path()) && pos!=std::string::npos)
      {
	if (pos+2<lsdir.size()) {
	  int ls = atoi(lsdir.substr(pos+2,std::string::npos).c_str());
	  if (ls && ls>lastOpenedLumi_ ) {
	    lsdirs.push_back(*it);
	  }
	}
      }
    }

    //check if raw files are not appearing in per-LS dir scheme
    if (!lsdirs.size() && currentDataDir_.string().empty())
    {
      bool foundRawDataInBaseDir = false;
      for ( boost::filesystem::directory_iterator it(runDirectory_);it != itEnd; ++it)
      {
	if (!boost::filesystem::is_directory(it->path()) && it->path().extension() == ".raw")
	{
          foundRawDataInBaseDir = true;
	  break;
	}
      }
      if (foundRawDataInBaseDir)
      {
	//start reading files in "bu" directory
	lastOpenedLumi_++;//mark beginning of lumi(it will be updated)
	currentDataDir_=runDirectory_;
	continue;
      }
    }

    if (!lsdirs.size() && !currentDataDir_.string().empty()) {
      //maybe run has ended
      if ( runEnded() ) return true;
      //file grab failed, but there is only one lumi dir and is active. keep looping for more data
      usleep(100000);
      continue;
    }
    
    //find next suitable LS directory
    if (lsdirs.size()) {
      std::sort(lsdirs.begin(), lsdirs.end());
      currentDataDir_ = lsdirs.at(0);
      lastOpenedLumi_ = atoi(currentDataDir_.string().substr(currentDataDir_.string().rfind("/")+3,std::string::npos).c_str());
      newLumiSection_=true;
      // MARK! input metafile merge
      if (lastOpenedLumi_ > 1) {
    	  // TODO re-enable
  	    //mergeInputMetafilesForLumi();
      }
      break;
    }
    //loop again
    usleep(100000);
  }
  while (1/*!edm::shutdown_flag*/);

  return true;

}

bool
FedRawDataInputSource::grabNextFile(boost::filesystem::path const& rawdir,boost::filesystem::path const& nextFile)
{
  std::vector<boost::filesystem::path> files;

  boost::filesystem::directory_iterator itEnd;
  //first look into last LS dir
  try
  {
    for ( boost::filesystem::directory_iterator it(rawdir.string());
          it != itEnd; ++it)
    {
      if ( it->path().extension() == ".raw" )
        files.push_back(*it);
    }
    
    if ( files.empty() )
    {
      return false;
    }
    else
    {
      std::sort(files.begin(), files.end());
      std::cout << " rename " << files.front() << " to " << nextFile << std::endl;
      boost::filesystem::rename(files.front(), nextFile);

      // MARK! grab json file too, if previous didn't throw

      /*
      boost::filesystem::path jsonSourcePath(files.front());
      boost::filesystem::path jsonDestPath(nextFile);
      boost::filesystem::path jsonExt(".jsn");
      jsonSourcePath.replace_extension(jsonExt);
      jsonDestPath.replace_extension(jsonExt);

      std::cout << " JSON rename " << jsonSourcePath << " to " << jsonDestPath << std::endl;

      boost::filesystem::rename(jsonSourcePath, jsonDestPath);
	  */

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
  workingDirectory_ = runBaseDirectory_;

  boost::filesystem::directory_iterator itEnd;
  bool foundHLTdir=false;
  for ( boost::filesystem::directory_iterator it(runBaseDirectory_);
      it != itEnd; ++it)
  {
    if ( boost::filesystem::is_directory(it->path()) &&
	it->path().string().find("/hlt") !=std::string::npos)
      foundHLTdir=true;
  }
  workingDirectory_ /= "hlt";
  if (!foundHLTdir)
    boost::filesystem::create_directories(workingDirectory_);
  workingDirectory_ /= myDir.str();
  std::cout << " workDir " << workingDirectory_ << "will be created." << std::endl;
  boost::filesystem::create_directories(workingDirectory_);
  workDirCreated_=true;
}

int
FedRawDataInputSource::mergeInputMetafilesForLumi() const
{
	string inputFolder = workingDirectory_.string();
	std::stringstream ss;
	ss << "inputLS" << lastOpenedLumi_ -1 << ".jsn";
	string outputFile = workingDirectory_.string() + "/" + ss.str();
	vector<string> inputJSONFilePaths;

	std::cout << " >>>>>>>>->>>>>>>> Input metafile merge for LS: " << lastOpenedLumi_ - 1 << "; input folder=" << inputFolder
			<< " outputFile=" << outputFile << std::endl;

	string outcomeString;
	FileIO::getFileList(inputFolder, inputJSONFilePaths, outcomeString, ".jsn", "^[0-9]+$");

	int outcome = JSONFileCollector::mergeFiles(inputJSONFilePaths, outputFile, false);

	// FIXME this fails on multiprocess
	// delete all input files
	for (auto a : inputJSONFilePaths)
		std::remove(a.c_str());

	return outcome;
}

// define this class as an input source
DEFINE_FWK_INPUT_SOURCE(FedRawDataInputSource);


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
