#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <stdio.h>

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
lastOpenedLumi_(0),
eorFileSeen_(false)
{
  buRunDirectory_ = boost::filesystem::path(pset.getUntrackedParameter<std::string>("rootBUDirectory"));
  findRunDir( pset.getUntrackedParameter<std::string>("rootFUDirectory") );
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
FedRawDataInputSource::findRunDir(const std::string& rootFUDirectory)
{
  // The run dir should be set via the configuration
  // For now, just grab the latest run directory available
  
  std::vector<boost::filesystem::path> dirs;
  boost::filesystem::directory_iterator itEnd;
  do {
    for ( boost::filesystem::directory_iterator it(rootFUDirectory);
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
  localRunDirectory_ = dirs.back();
  localRunBaseDirectory_ = dirs.back();

  std::string rnString = localRunDirectory_.string().substr(localRunDirectory_.string().find("run")+3);
  runNumber_ = atoi(rnString.c_str());

  // get the corresponding BU dir
  buRunDirectory_ /= localRunDirectory_.filename();

  /*
  if (boost::filesystem::exists(buRunDirectory_)) {
	  //find "bu" subdir and take files from there, if present
	  for ( boost::filesystem::directory_iterator it(buRunDirectory_);it != itEnd; ++it)
	  {
		  if ( boost::filesystem::is_directory(it->path()) && boost::algorithm::ends_with(it->path().string(),"/bu"))
		  {
			  buRunDirectory_=*it;
			  break;
		  }
	  }
  }
  else
  {
	  std::cout << "BU INPUT DIRECTORY DOES NOT EXIST @: " << buRunDirectory_.string() << std::endl;
  }
  */

  edm::LogInfo("FedRawDataInputSource") << "Getting data from " << buRunDirectory_.string();

}

bool FedRawDataInputSource::checkNextEvent()
{

  //std::cout << ">>>>>>>>>>>>>>>> Checking next event" << std::endl;
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
    //if (workDirCreated_)
    //  boost::filesystem::remove(workingDirectory_);
    return false;
  }

  //same lumi, or new lumi detected in file (old mode)
  if (!newLumiSection_)
  {
    fread((void*)&eventHeader, sizeof(uint32_t), 4, fileStream_);
    // FIXME: fails with real BU
    //assert( eventHeader.version == 2 );

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
    //std::cout << ">>>>>>>>>>>>>>>> Set event cached" << std::endl;
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
  //std::cout << ">>>>>>>>>>>>>>>> Reading next event" << std::endl;
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
  //std::cout << ">>>>>>>>>>>>>>>> FREAD from file stream" << std::endl;
  fread((void*)fedSizes, sizeof(uint32_t), 1024, fileStream_);
  //std::cout << ">>>>>>>>>>>>>>>> OK" << std::endl;
  for (unsigned int i=0;i<1024;i++) {
    totalEventSize += fedSizes[i];
  }
  
  unsigned int gtpevmsize = fedSizes[FEDNumbering::MINTriggerGTPFEDID];
  if ( gtpevmsize > 0 )
    evf::evtn::evm_board_setformat(gtpevmsize);
  
  char* event = new char[totalEventSize];
  //std::cout << ">>>>>>>>>>>>>>>> FREAD totalEventSize from file stream" << std::endl;
  fread((void*)event, totalEventSize, 1, fileStream_);
  //std::cout << ">>>>>>>>>>>>>>>> OK" << std::endl;
  
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
  delete event;
  return tstamp;
}


bool
FedRawDataInputSource::openNextFile()
{
  if (!workDirCreated_) createWorkingDirectory();
  //boost::filesystem::path nextFile = workingDirectory_;
  boost::filesystem::path nextFile = buRunDirectory_;
  std::ostringstream fileName;
  char thishost[256];
  gethostname(thishost,255);
  fileName << std::setfill('0') << std::setw(16) << fileIndex_++ << "_" << thishost << "_" << getpid() << ".raw";
  nextFile /= fileName.str();

  openFile(nextFile);//closes previous file


  while(!searchForNextFile(nextFile) && !eorFileSeen_) {
	  std::cout << "No file for me... sleep and try again..." << std::endl;
	  usleep(250000);
  }

  return ( fileStream_ != 0 || !eorFileSeen_);
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
FedRawDataInputSource::searchForNextFile(
		boost::filesystem::path const& nextFile) {
	newLumiSection_ = false;
	bool fileIsOKToGrab = false;
	std::stringstream ss;
	unsigned int ls, index;

	std::cout << "Asking for next file... to the DaqDirector" << std::endl;
	fileIsOKToGrab = edm::Service<evf::EvFDaqDirector>()->updateFuLock(ls,
			index, eorFileSeen_);

	if (fileIsOKToGrab) {
		ss << buRunDirectory_.string() << "/ls" << std::setfill('0')
				<< std::setw(4) << ls << "_" << std::setfill('0') << std::setw(
				6) << index << ".raw";
		string path = ss.str();
		std::cout << "The director says to grab: " << path << std::endl;
		std::cout << "grabbin next file" << std::endl;
		boost::filesystem::path theFileToGrab(ss.str());
		if (grabNextFile(theFileToGrab, nextFile)) {
			return true;
		} else {
			std::cout << "GRABBING SHOULD NEVER FAIL! THE HORROOOOOOOOOOOOOOR!" << std::endl;
			//return false;
			exit(0);
		}
	} else {
		std::cout << "The DAQ Director has nothing for me! " << std::endl;
		if (eorFileSeen_) {
			std::cout << "...and he's seen the end of run file!" << std::endl;
		}
		return false;
	}
}

bool
FedRawDataInputSource::grabNextFile(boost::filesystem::path& file, boost::filesystem::path const& nextFile)
{
	try
	{
		// TODO remove this rename business
		/*
		std::cout << "0. RENAME @ BU " << file.string()  << " to " << nextFile << std::endl;
		int rc = rename(file.string().c_str(), nextFile.string().c_str());
		if (rc != 0) {
			throw std::runtime_error("Cannot RENAME DATA file, rc is != 0!");
		}
		*/

		// MARK! grab json file too, if previous didn't throw
		// assemble json path on /hlt/data
		boost::filesystem::path nextFileJson = workingDirectory_;
		//std::ostringstream fileName;
		//nextFile /= fileName.str();
		boost::filesystem::path jsonSourcePath(file);
		boost::filesystem::path jsonDestPath(nextFileJson);
		boost::filesystem::path jsonExt(".jsn");
		jsonSourcePath.replace_extension(jsonExt);

		boost::filesystem::path jsonTempPath(jsonDestPath);

		//jsonTempPath.remove_filename();
		std::ostringstream fileNameWithPID;
		fileNameWithPID << jsonSourcePath.stem().string() << "_" << getpid() << ".jsn";
		boost::filesystem::path filePathWithPID(fileNameWithPID.str());
		//jsonTempPath /= jsonSourcePath.filename();
		jsonTempPath /= filePathWithPID;
		std::cout << " JSON rename " << jsonSourcePath << " to " << jsonTempPath << std::endl;

		//boost::filesystem::rename(jsonSourcePath, jsonTempPath);
		//move JSON
		string mvCmd = "mv " + jsonSourcePath.string() + " " + jsonTempPath.string();
		std::cout << " Running copy cmd = " << mvCmd << std::endl;
		int rc = system(mvCmd.c_str());
		std::cout << " return code = " << rc << std::endl;
		if (rc != 0) {
			throw std::runtime_error("Cannot copy JSON file, rc is != 0!");
		}

		//openFile(nextFile);
		openFile(file);

		return true;
	}

	catch (const boost::filesystem::filesystem_error& ex)
	{
		// Input dir gone?
		std::cout << "BOOST FILESYSTEM ERROR CAUGHT: " << ex.what() << std::endl;
		std::cout << "Maybe the BU run dir disappeared? Ending process with code 0..." << std::endl;
		exit(0);
	}
	catch (std::runtime_error e)
	{
		// Another process grabbed the file and NFS did not register this
		std::cout << "Exception text: " << e.what() << std::endl;
	}
	catch (std::exception e)
	{
		// BU run directory disappeared?
		std::cout << "SOME OTHER EXCEPTION OCCURED!!!! ->" << e.what() << std::endl;
	}
	return false;
}


bool
FedRawDataInputSource::runEnded() const
{
  boost::filesystem::path endOfRunMarker = buRunDirectory_;
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

  // host_pid dir
  //myDir << std::setfill('0') << std::setw(5) << thishost << "_" << getpid();

  // host_DATA dir
  myDir << std::setfill('0') << std::setw(5) << thishost << "_DATA";
  workingDirectory_ = localRunBaseDirectory_;

  boost::filesystem::directory_iterator itEnd;
  bool foundHLTdir=false;
  bool foundHostDir = false;
  for ( boost::filesystem::directory_iterator it(localRunBaseDirectory_);
      it != itEnd; ++it)
  {
    if ( boost::filesystem::is_directory(it->path()) &&
	it->path().string().find("/hlt") !=std::string::npos)
      foundHLTdir=true;
  }
  workingDirectory_ /= "hlt";
  if (!foundHLTdir) {
    boost::filesystem::create_directories(workingDirectory_);
  }

  for ( boost::filesystem::directory_iterator it(workingDirectory_);
        it != itEnd; ++it)
    {
      if ( boost::filesystem::is_directory(it->path()) &&
  	it->path().string().find(myDir.str()) !=std::string::npos)
      foundHostDir=true;
    }

  workingDirectory_ /= myDir.str();

  if (!foundHostDir) {
	  std::cout << " workDir " << workingDirectory_ << "will be created." << std::endl;
	  boost::filesystem::create_directories(workingDirectory_);
  }
  workDirCreated_=true;

  // also create MON directory
  std::ostringstream monDir;
  monDir << std::setfill('0') << std::setw(5) << thishost << "_MON";
  boost::filesystem::path monDirectory = localRunBaseDirectory_;

  monDirectory /= "hlt";
  monDirectory /= monDir.str();

  // now at /root/run/hlt/host_MON

  bool foundMonDir = false;
  if ( boost::filesystem::is_directory(monDirectory))
	  foundMonDir=true;
  if (!foundMonDir) {
	  std::cout << "<MON> DIR NOT FOUND! CREATING!!!" << std::endl;
	  boost::filesystem::create_directories(monDirectory);
  }

}


// define this class as an input source
DEFINE_FWK_INPUT_SOURCE(FedRawDataInputSource);


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
