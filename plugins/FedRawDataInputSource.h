#ifndef EventFilter_Utilities_FedRawDataInputSource_h
#define EventFilter_Utilities_FedRawDataInputSource_h

#include <memory>
#include <stdio.h>

#include "boost/filesystem.hpp"

#include "DataFormats/Provenance/interface/Timestamp.h"
#include "FWCore/Sources/interface/RawInputSource.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Sources/interface/DaqProvenanceHelper.h"

#include "../interface/JsonMonitorable.h"
#include "../interface/DataPointMonitor.h"
#include "../interface/JSONSerializer.h"

using namespace jsoncollector;

class FEDRawDataCollection;
class InputSourceDescription;
class ParameterSet;


class FedRawDataInputSource : public edm::RawInputSource
{
  
public:
  explicit FedRawDataInputSource(edm::ParameterSet const&, edm::InputSourceDescription const&);
  virtual ~FedRawDataInputSource();
  
protected:
  virtual bool checkNextEvent();
  virtual edm::EventPrincipal * read(edm::EventPrincipal& eventPrincipal);

private:
  virtual void preForkReleaseResources();
  virtual void postForkReacquireResources(boost::shared_ptr<edm::multicore::MessageReceiverForSource>);
  virtual void rewind_();

  void createWorkingDirectory();
  void findRunDir(const std::string& rootFUDirectory);
  edm::Timestamp fillFEDRawDataCollection(std::auto_ptr<FEDRawDataCollection>&);
  bool openNextFile();
  void openFile(boost::filesystem::path const&);
  bool searchForNextFile(boost::filesystem::path const&);
  //bool grabNextFile(boost::filesystem::path const&,boost::filesystem::path const&);
  bool grabNextFile(std::vector<boost::filesystem::path>&,boost::filesystem::path const&,std::vector<boost::filesystem::path>&);
  bool eofReached() const;
  bool runEnded() const;



  edm::DaqProvenanceHelper daqProvenanceHelper_;

  // the BU run directory
  boost::filesystem::path buRunDirectory_;
  // the OUTPUT run directory
  boost::filesystem::path localRunBaseDirectory_;
  boost::filesystem::path localRunDirectory_;
  edm::RunNumber_t runNumber_;

  boost::filesystem::path workingDirectory_;
  boost::filesystem::path openFile_;
  size_t fileIndex_;
  FILE* fileStream_;
  bool workDirCreated_;
  edm::EventID eventID_;

  bool newLumiSection_;
  int lastOpenedLumi_;
  boost::filesystem::path currentDataDir_;

};

#endif // EventFilter_Utilities_FedRawDataInputSource_h


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
