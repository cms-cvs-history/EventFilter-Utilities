#ifndef EventFilter_Utilities_FedRawDataInputSource_h
#define EventFilter_Utilities_FedRawDataInputSource_h

#include <memory>
#include <stdio.h>

#include "boost/filesystem.hpp"

#include "DataFormats/Provenance/interface/Timestamp.h"
#include "FWCore/Sources/interface/RawInputSource.h"


class FEDRawDataCollection;
class InputSourceDescription;
class ParameterSet;

class FedRawDataInputSource : public edm::RawInputSource
{
  
public:
  explicit FedRawDataInputSource(edm::ParameterSet const&, edm::InputSourceDescription const&);
  virtual ~FedRawDataInputSource();
  
protected:
  virtual std::auto_ptr<edm::Event> readOneEvent();

private:
  edm::Timestamp fillFEDRawDataCollection(std::auto_ptr<FEDRawDataCollection>&);
  bool openNextFile();
  void openFile(boost::filesystem::path const&);
  void grabNextFile(boost::filesystem::path const&);
  bool eofReached() const;
  bool runEnded() const;

  const boost::filesystem::path runDirectory_;
  boost::filesystem::path workingDirectory_;
  size_t fileIndex_;
  FILE* fileStream_;
  
};

#endif // EventFilter_Utilities_FedRawDataInputSource_h


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
