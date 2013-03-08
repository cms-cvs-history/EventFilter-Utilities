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
lastOpenedLumi_(0)
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

  edm::LogInfo("FedRawDataInputSource") << "Getting data from " << buRunDirectory_.string();

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
  fileName << std::setfill('0') << std::setw(16) << fileIndex_++ << "_" << thishost << "_" << getpid() << ".rawg";
  nextFile /= fileName.str();

  // write previous json
  /*
  boost::filesystem::path jsonFile = workingDirectory_;
  std::ostringstream fileNameJson;
  fileNameJson << std::setfill('0') << std::setw(16) << fileIndex_ << ".json";
  jsonFile /= fileNameJson.str();
  */

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
	std::vector<boost::filesystem::path> files;
	std::vector<boost::filesystem::path> badFiles;
	boost::filesystem::directory_iterator itEnd;

	do
	{
		currentDataDir_ = buRunDirectory_;
		files.clear();
		for ( boost::filesystem::directory_iterator it(currentDataDir_.string());
				it != itEnd; ++it)
		{
			if ( it->path().extension() == ".raw")
			{
				bool fileIsBad = false;
				for (unsigned int i = 0; i < badFiles.size(); i++)
				{
					std::cout << "Comparing against bad file list: " << badFiles[i].string() << " / " << it->path().string() << std::endl;
					if ((it->path() == badFiles[i]))
					{
						std::cout << "FILE IS BAD = true" <<std::endl;
						fileIsBad = true;
						break;
					}
				}
				if (fileIsBad)
					std::cout << "Ignoring bad file =" << it->path().string() << std::endl;
				else
					files.push_back(*it);
			}
		}
		if (files.size() > 0)
		{
			std::cout << "grabbin next file" <<std::endl;
			if (grabNextFile(files,nextFile,badFiles))
			{
				badFiles.clear();
				return true;
			}
		}
		else {
			std::cout << "NOTHING TO GRAB! ... retrying ..." << std::endl;
		}

		//loop again
		usleep(300000);
	}
	while (1/*!edm::shutdown_flag*/);

	return true;
}

bool
FedRawDataInputSource::grabNextFile(std::vector<boost::filesystem::path>& files,boost::filesystem::path const& nextFile, std::vector<boost::filesystem::path>& badFiles)
{
	try
	{
		std::sort(files.begin(), files.end());
		std::cout << " rename " << files.front() << " to " << nextFile << std::endl;

		//RENAME
		std::cout << "0. RENAME @ BU " << files.front().string()  << " to " << nextFile << std::endl;
		int rc = rename(files.front().string().c_str(), nextFile.string().c_str());
		if (rc != 0) {
			std::cout << "BAD FILE FOUND!!!!!! " << files.front().string() << std::endl;
			badFiles.push_back(files.front());
			throw std::runtime_error("Cannot RENAME DATA file, rc is != 0!");
		}


		// FIXME DOESN'T WORK OVER NFS!!!!!!
		//boost::filesystem::rename(files.front(), nextFile);

		//GET FD
		/*
		std::cout << "1. Opening file to get FD = " << files.front().string() << std::endl;
		int fd = open(files.front().string().c_str(), O_WRONLY);
		if (fd == -1) {
			badFiles.push_back(files.front());
			throw std::runtime_error("Cannot open DATA file, FD = -1!");
		}

		//LOCK
		std::cout << "2. Locking NONBLOCKING file = " << files.front().string() << std::endl;
		int result = flock(fd, LOCK_EX | LOCK_NB);
		if (result == -1) {
			badFiles.push_back(files.front());
			throw std::runtime_error("Cannot lock DATA file, flock result is = -1!");
		}

		//MOVE
		std::cout << "3. MOVE TO FU. From " << files.front().string()  << " to " << nextFile.string() << std::endl;
		string mvCmd = "mv " + files.front().string() + " " + nextFile.string();
		std::cout << " Running move cmd = " << mvCmd << std::endl;
		int rc = system(mvCmd.c_str());
		std::cout << " return code = " << rc << std::endl;
		if (rc != 0) {
			//badFiles.push_back(files.front());
			//throw std::runtime_error("Cannot move DATA file, rc is != 0! THIS SHOULD NEVER HAPPEN! Exiting...");
			std::cout << "MOVE TO FU FAILED! THIS SHOULD NEVER HAPPEN IF LOCKING WORKS! EXITING..." << std::endl;
			exit(-1);
		}

		//to avoid .nfs leftover files
		close(fd);
		*/

		// LOCK USING FCNTL

		//struct flock fl;
		//int fd;

		//fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
		//fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
		//fl.l_start  = 0;        /* Offset from l_whence         */
		//fl.l_len    = 0;        /* length, 0 = to EOF           */
		//fl.l_pid    = getpid(); /* our PID                      */
		/*
    
		fd = open(files.front().string().c_str(), O_WRONLY);
		std::cout << "0. FD is =   " << fd << std::endl;

		std::cout << "1. LOCK  " << files.front().string() << std::endl;
		int rc = fcntl(fd, F_SETLK, &fl);  // F_GETLK, F_SETLK, F_SETLKW
		if (errno == EAGAIN) std::cout << "EAGAIN"<< std::endl;
		if (errno == EACCES) std::cout << "EACCESS"<< std::endl;
		if (errno == EBADF) std::cout << "EBADF"<< std::endl;
		if (errno == EDEADLK) std::cout << "EDEADLK"<< std::endl;
		if (errno == EFAULT) std::cout << "EFAULT"<< std::endl;
		if (errno == EINTR) std::cout << "EINTR"<< std::endl;
		if (errno == EINVAL) std::cout << "EINVAL"<< std::endl;
		if (errno == EMFILE) std::cout << "EMFILE"<< std::endl;
		if (errno == ENOLOCCK) std::cout << "ENLOCK"<< std::endl;
		if (errno == EPERM) std::cout << "EPERM"<< std::endl;
		if (rc == -1) {
			badFiles.push_back(files.front());
			throw std::runtime_error("Cannot LOCK data file, rc is != 0!");
		}
		*/

		//std::cout << "Sleeping for 1 MINUTE!!!" << std::endl;
		//::sleep(60);

		// OPEN
		/*
		 std::cout << "3. TRY TO OPEN  " << files.front().string() << std::endl;
		 std::ifstream grabbedFile(files.front().string());
		 if (grabbedFile.is_open()) {
		 grabbedFile.close();
		 }
		 else {
		 // the file was not really grabbed...
		 badFiles.push_back(files.front());
		 throw std::runtime_error("Cannot OPEN data file file! This should never happed!!");
		 exit(-1);
		 }
		 */

		//COPY
		/*
		 std::cout << "4. COPY TO FU. From " << files.front().string()  << " to " << nextFile.string() << std::endl;
		 string copyCmd = "cp " + files.front().string() + " " + nextFile.string();
		 std::cout << " Running copy cmd = " << copyCmd << std::endl;
		int rc = system(copyCmd.c_str());
		std::cout << " return code = " << rc << std::endl;
		if (rc != 0) {
			badFiles.push_back(files.front());
			throw std::runtime_error("Cannot copy DATA file, rc is != 0!");
		}
		*/

		//UNLOCK
		/*
		std::cout << "6. UNLOCKING file = " << files.front().string() << std::endl;
		result = flock(fd, LOCK_UN);
		if (result == -1) {
			badFiles.push_back(files.front());
			throw std::runtime_error("Cannot UNLOCK DATA file, flock result is = -1!");
		}
		*/

		// REMOVE FROM BU
		/*
		string rmCmd = "rm " + renamed;
		std::cout << "7.  REMOVE FROM BU = " << rmCmd << std::endl;
		rc = system(rmCmd.c_str());
		std::cout << " return code = " << rc << std::endl;
		if (rc != 0) {
			badFiles.push_back(files.front());
			// ALSO NEED TO REMOVE COPIED FILE (FU)
			string rmInitialFileCmd = "rm " + nextFile.string();
			system(rmInitialFileCmd.c_str());
			throw std::runtime_error("Cannot REMOVE DATA file, rc is != 0!");
		}
		*/

		// MARK! grab json file too, if previous didn't throw
		// assemble json path on /hlt/data
		boost::filesystem::path nextFileJson = workingDirectory_;
		//std::ostringstream fileName;
		//nextFile /= fileName.str();
		boost::filesystem::path jsonSourcePath(files.front());
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
		rc = system(mvCmd.c_str());
		std::cout << " return code = " << rc << std::endl;
		if (rc != 0) {
			throw std::runtime_error("Cannot copy JSON file, rc is != 0!");
		}

		openFile(nextFile);
		return true;
	}

	catch (const boost::filesystem::filesystem_error& ex)
	{
		// Input dir gone?
		std::cout << "BOOST FILESYSTEM ERROR CAUGHT: " << ex.what() << std::endl;
		std::cout << "Maybe the BU run dir disappeared? Ending process..." << std::endl;
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
    // MARK! TODO remove
    //std::cout << "sleeping for 2 sec ------> THIS HAS TO BE REMOVED!" << std::endl;
    //sleep(2);
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
