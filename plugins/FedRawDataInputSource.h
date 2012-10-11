#ifndef EventFilter_Utilities_FedRawDataInputSource_h
#define EventFilter_Utilities_FedRawDataInputSource_h

#include <fstream>
#include <memory>

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
  void openNextFile();
  void grabNextFile(boost::filesystem::path const& nextFile);
  bool runEnded() const;

  const boost::filesystem::path runDirectory_;
  boost::filesystem::path workingDirectory_;
  size_t fileIndex_;
  std::ifstream finput_;
  
};

#endif // EventFilter_Utilities_FedRawDataInputSource_h


/// emacs configuration
/// Local Variables: -
/// mode: c++ -
/// c-basic-offset: 2 -
/// indent-tabs-mode: nil -
/// End: -
