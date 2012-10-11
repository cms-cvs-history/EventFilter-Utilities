#ifndef IOPool_Streamer_RecoEventOutputModuleForFU_h
#define IOPool_Streamer_RecoEventOutputModuleForFU_h

#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "IOPool/Streamer/interface/StreamerOutputModuleBase.h"
#include "FWCore/Framework/interface/LuminosityBlockPrincipal.h"

#include <sstream>
#include <iomanip>

namespace evf {
  template<typename Consumer>
  class RecoEventOutputModuleForFU : public edm::StreamerOutputModuleBase {
    
    /** Consumers are supposed to provide
	void doOutputHeader(InitMsgBuilder const& init_message)
	void doOutputEvent(EventMsgBuilder const& msg)
	void start()
	void stop()
	static void fillDescription(ParameterSetDescription&)
    **/
    
  public:
    explicit RecoEventOutputModuleForFU(edm::ParameterSet const& ps);  
    virtual ~RecoEventOutputModuleForFU();
    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
    
  private:
    virtual void start() const;
    virtual void stop() const;
    virtual void doOutputHeader(InitMsgBuilder const& init_message) const;
    virtual void doOutputEvent(EventMsgBuilder const& msg) const;
    //    virtual void beginRun(edm::RunPrincipal const&);
    virtual void beginLuminosityBlock(edm::LuminosityBlockPrincipal const&);
    void         initializeStreams(){
      if ( baseDir_.empty() )
        smpath_ = edm::Service<evf::EvFDaqDirector>()->getPathForFU();
      else
        smpath_ = baseDir_;
      events_base_filename_ = smpath_+"/"+stream_label_;
    }

  private:
    std::auto_ptr<Consumer> c_;
    std::string stream_label_;
    std::string events_base_filename_;
    std::string baseDir_;
    std::string smpath_;
  }; //end-of-class-def

  template<typename Consumer>
  RecoEventOutputModuleForFU<Consumer>::RecoEventOutputModuleForFU(edm::ParameterSet const& ps) :
    edm::StreamerOutputModuleBase(ps),
    c_(new Consumer(ps)),
    stream_label_(ps.getParameter<std::string>("@module_label")),
    baseDir_(ps.getUntrackedParameter<std::string>("baseDir",""))
      {
        initializeStreams();
      }

  template<typename Consumer>
  RecoEventOutputModuleForFU<Consumer>::~RecoEventOutputModuleForFU() {}

  template<typename Consumer>
  void
  RecoEventOutputModuleForFU<Consumer>::start() const
  {
    std::cout << "RecoEventOutputModuleForFU: start() method " << std::endl;

    std::string init_filename_ = smpath_+"/"+stream_label_+".ini"; 
    std::string eof_filename_ = smpath_+"/"+stream_label_+".eof";
   
    std::string first =  events_base_filename_ + "000001.dat";
    std::cout << "RecoEventOutputModuleForFU, initializing streams. init stream: " 
	      <<init_filename_ << " eof stream " << eof_filename_ << " event stream " << first << std::endl;

    c_->setOutputFiles(init_filename_,eof_filename_);
    c_->start();
  }
  
  template<typename Consumer>
  void
  RecoEventOutputModuleForFU<Consumer>::stop() const {
    c_->stop();
  }

  template<typename Consumer>
  void
  RecoEventOutputModuleForFU<Consumer>::doOutputHeader(InitMsgBuilder const& init_message) const {
    c_->doOutputHeader(init_message);
  }
   
//______________________________________________________________________________
  template<typename Consumer>
  void
  RecoEventOutputModuleForFU<Consumer>::doOutputEvent(EventMsgBuilder const& msg) const {
    c_->doOutputEvent(msg); // You can't use msg in RecoEventOutputModuleForFU after this point
  }

  template<typename Consumer>
  void
  RecoEventOutputModuleForFU<Consumer>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    edm::StreamerOutputModuleBase::fillDescription(desc);
    Consumer::fillDescription(desc);
    desc.addUntracked<std::string>("baseDir", "")
        ->setComment("Top level output directory");
    descriptions.add("streamerOutput", desc);
  }

//   template<typename Consumer>
//   void RecoEventOutputModuleForFU<Consumer>::beginRun(edm::RunPrincipal const &run){


//   }

  template<typename Consumer>
  void RecoEventOutputModuleForFU<Consumer>::beginLuminosityBlock(edm::LuminosityBlockPrincipal const &ls){
    std::cout << "RecoEventOutputModuleForFU : begin lumi " << std::endl;
    std::ostringstream ost;
    ost << events_base_filename_  << std::setfill('0') << std::setw(6) << ls.luminosityBlock() << ".dat";
    std::string filename = ost.str();
    c_->setOutputFile(filename);
  }
} // end of namespace-edm

#endif
