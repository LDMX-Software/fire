/**
 * @file Process.cxx
 * Implementation file for Process class
 */

#include "fire/factory/Factory.hpp"
#include "fire/Process.h"
#include <iostream>
#include "fire/Event.h"
#include "fire/EventFile.h"
#include "fire/EventProcessor.h"
#include "fire/Exception/Exception.h"
#include "fire/Logger.h"
#include "fire/NtupleManager.h"
#include "fire/RunHeader.h"

namespace fire {

Process::Process(const fire::config::Parameters &configuration)
    : conditions_{*this} {

  config_ = configuration; 

  passname_ = configuration.getParameter<std::string>("passName", "");
  histoFilename_ = configuration.getParameter<std::string>("histogramFile", "");
  logFileName_ = configuration.getParameter<std::string>("logFileName", "");
  outputFile_ = configuration.getParameter<std::string>("outputFiles", "");

  maxTries_ = configuration.getParameter<int>("maxTriesPerEvent", 1);
  eventLimit_ = configuration.getParameter<int>("maxEvents", -1);
  logFrequency_ = configuration.getParameter<int>("logFrequency", -1);
  compressionSetting_ =
      configuration.getParameter<int>("compressionSetting", 9);
  termLevelInt_ = configuration.getParameter<int>("termLogLevel", 2);
  fileLevelInt_ = configuration.getParameter<int>("fileLogLevel", 0);

  inputFiles_ =
      configuration.getParameter<std::vector<std::string>>("inputFiles", {});
  dropKeepRules_ =
      configuration.getParameter<std::vector<std::string>>("keep", {});

  eventHeader_ = 0;

  auto run{configuration.getParameter<int>("run", -1)};
  if (run > 0) runForGeneration_ = run;

  auto libs{
      configuration.getParameter<std::vector<std::string>>("libraries", {})};
  std::for_each(libs.begin(), libs.end(), [](auto &lib) {
      factory::loadLibrary(lib);
  });

  storageController_.setDefaultKeep(
      configuration.getParameter<bool>("skimDefaultIsKeep", true));
  auto skimRules{
      configuration.getParameter<std::vector<std::string>>("skimRules", {})};
  for (size_t i = 0; i < skimRules.size(); i += 2) {
    storageController_.addRule(skimRules[i], skimRules[i + 1]);
  }

  auto sequence{
      configuration.getParameter<std::vector<fire::config::Parameters>>(
          "sequence", {})};
  if (sequence.empty() &&
      configuration.getParameter<bool>("testingMode", false)) {
    EXCEPTION_RAISE(
        "NoSeq",
        "No sequence has been defined. What should I be doing?\nUse "
        "p.sequence to tell me what processors to run.");
  }
  for (auto proc : sequence) {
    auto className{proc.getParameter<std::string>("className")};
    auto instanceName{proc.getParameter<std::string>("instanceName")};
    std::unique_ptr<EventProcessor> ep;
    try {
      ep = EventProcessor::Factory::get().make(className, instanceName, *this);
    } catch (Exception const& e) {
      EXCEPTION_RAISE(
          "UnableToCreate",
          "Unable to create instance '" + instanceName + "' of class '" +
              className +
              "'. Did you load the library that this class is apart of?");
    }
    ep->configure(proc);
    sequence_.push_back(ep);
  }

  auto conditionsObjectProviders{
      configuration.getParameter<std::vector<fire::config::Parameters>>(
          "conditionsObjectProviders", {})};
  for (auto cop : conditionsObjectProviders) {
    auto className{cop.getParameter<std::string>("className")};
    auto objectName{cop.getParameter<std::string>("objectName")};
    auto tagName{cop.getParameter<std::string>("tagName")};

    conditions_.createConditionsObjectProvider(className, objectName, tagName,
                                               cop);
  }
}

void Process::run() {
  // set up the logging for this run
  logging::open(logging::convertLevel(termLevelInt_),
                logging::convertLevel(fileLevelInt_),
                logFileName_  // if this is empty string, no file is logged to
  );

  // Counter to keep track of the number of events that have been
  // procesed
  auto n_events_processed{0};

  // event bus for this process
  Event theEvent(passname_);

  // Start by notifying everyone that modules processing is beginning
  conditions_.onProcessStart();
  for (auto module : sequence_) module->onProcessStart();

  // If we have no input files, but do have an event number, run for
  // that number of events and generate an output file.
  if (inputFiles_.empty() && eventLimit_ > 0) {
    if (outputFile_.empty()) {
      EXCEPTION_RAISE("InvalidConfig",
                      "No input files or output file were given.");
    }

    // Configure the event file to create an output file with no parent. This
    // requires setting the parameters isOutputFile and isSingleOutput to true.
    EventFile outFile(config_, outputFile_, nullptr, true, true, false); 

    for (auto module : sequence_) module->onFileOpen(outFile);

    outFile.setupEvent(&theEvent);

    for (auto rule : dropKeepRules_) outFile.addDrop(rule);

    ldmx::RunHeader runHeader(runForGeneration_);
    runHeader.setRunStart(std::time(nullptr));  // set run starting
    runHeader_ = &runHeader;            // give handle to run header to process
    outFile.writeRunHeader(runHeader);  // add run header to file

    newRun(runHeader);

    int numTries = 0;  // number of tries for the current event number
    while (n_events_processed < eventLimit_) {
      ldmx::EventHeader &eh = theEvent.getEventHeader();
      eh.setRun(runForGeneration_);
      eh.setEventNumber(n_events_processed + 1);
      eh.setTimestamp(TTimeStamp());

      // event header pointer grab
      eventHeader_ = theEvent.getEventHeaderPtr();

      numTries++;

      // reset the storage controller state
      storageController_.resetEventState();

      bool completed = process(n_events_processed,theEvent);

      outFile.nextEvent(storageController_.keepEvent(completed));

      if (completed or numTries >= maxTries_) {
        n_events_processed++;                 // increment events made
        NtupleManager::getInstance().fill();  // fill ntuples
        numTries = 0;                         // reset try counter
      }

      NtupleManager::getInstance().clear();
    }

    for (auto module : sequence_) module->onFileClose(outFile);

    runHeader.setRunEnd(std::time(nullptr));
    ldmx_log(info) << runHeader;
    outFile.close();

  } else {
    // there are input files

    EventFile *outFile(0);

    bool singleOutput = true;

    // next, loop through the files
    int ifile = 0;
    int wasRun = -1;
    for (auto infilename : inputFiles_) {
      EventFile inFile(config_, infilename);

      ldmx_log(info) << "Opening file " << infilename;

      for (auto module : sequence_) module->onFileOpen(inFile);

      // configure event file that will be iterated over
      EventFile *masterFile;
      if (!outputFile_.empty()) {
        // setup new output file if either
        // 1) we are not in single output mode
        // 2) this is the first input file
        if (!singleOutput or ifile == 0) {
          // setup new output file
          outFile = new EventFile(config_, outputFile_, &inFile, singleOutput);
          ifile++;

          outFile->setupEvent(&theEvent);
          masterFile = outFile;

          for (auto rule : dropKeepRules_) outFile->addDrop(rule);

        } else {
          // all other input files
          outFile->updateParent(&inFile);
          masterFile = outFile;

        }  // check if in singleOutput mode

      } else {
        // empty output file list, use inputFile as master file
        inFile.setupEvent(&theEvent);
        masterFile = &inFile;
      }

      bool event_completed = true;
      while (masterFile->nextEvent(storageController_.keepEvent(event_completed))
             && (eventLimit_ < 0 || (n_events_processed) < eventLimit_)) {
        // clean up for storage control calculation
        storageController_.resetEventState();

        // event header pointer grab
        eventHeader_ = theEvent.getEventHeaderPtr();

        // notify for new run if necessary
        if (theEvent.getEventHeader().getRun() != wasRun) {
          wasRun = theEvent.getEventHeader().getRun();
          try {
            auto runHeader = masterFile->getRunHeader(wasRun);
            runHeader_ = &runHeader;  // save current run header for later
            ldmx_log(info) << "Got new run header from '"
                           << masterFile->getFileName() << "' ...\n"
                           << runHeader;
            newRun(runHeader);
          } catch (const fire::exception::Exception &) {
            ldmx_log(warn) << "Run header for run " << wasRun
                           << " was not found!";
          }
        }

        event_completed = process(n_events_processed,theEvent);

        if (event_completed) NtupleManager::getInstance().fill();
        NtupleManager::getInstance().clear();

        n_events_processed++;
      }  // loop through events

      if (eventLimit_ > 0 && n_events_processed == eventLimit_) {
        ldmx_log(info) << "Reached event limit of " << eventLimit_ << " events";
      }

      if (eventLimit_ == 0 && n_events_processed > eventLimit_) {
        ldmx_log(warn) << "Processing interrupted";
      }

      ldmx_log(info) << "Closing file " << infilename;

      for (auto module : sequence_) module->onFileClose(inFile);

      inFile.close();

      // Reset the event in case of multiple input files
      theEvent.onEndOfFile();

      if (outFile and !singleOutput) {
        outFile->close();
        delete outFile;
        outFile = nullptr;
      }

    }  // loop through input files

    if (outFile) {
      // close outFile
      //  outFile would survive to here in single output mode
      outFile->close();
      delete outFile;
      outFile = nullptr;
    }

  }  // are there input files? if-else tree

  // close up histogram file if anything was put into it
  if (histoTFile_) {
    histoTFile_->Write();
    delete histoTFile_;
    histoTFile_ = 0;
  }

  // finally, notify everyone that we are stopping
  for (auto module : sequence_) {
    module->onProcessEnd();
  }

  // we're done so let's close up the logging
  logging::close();
}

int Process::getRunNumber() const {
  return (eventHeader_) ? (eventHeader_->getRun()) : (runForGeneration_);
}

void Process::newRun(ldmx::RunHeader& header) {
  // Producers are allowed to put parameters into
  // the run header through 'beforeNewRun' method
  for (auto module : sequence_)
    if (dynamic_cast<Producer *>(module))
      dynamic_cast<Producer *>(module)->beforeNewRun(header);
  // now run header has been modified by Producers,
  // it is valid to read from for everyone else in 'onNewRun'
  conditions_.onNewRun(header);
  for (auto module : sequence_)
    module->onNewRun(header);
}

bool Process::process(int n, Event& event) const {
  if ((logFrequency_ != -1) &&
      ((n+1) % logFrequency_ == 0)) {
    TTimeStamp t;
    ldmx_log(info) << "Processing " << n+1 << " Run "
                   << event.getEventHeader().getRun() << " Event "
                   << event.getEventHeader().getEventNumber() << "  ("
                   << t.AsString("lc") << ")";
  }

  try {
    for (auto module : sequence_) {
      if (dynamic_cast<Producer *>(module)) {
        (dynamic_cast<Producer *>(module))->produce(event);
      } else if (dynamic_cast<Analyzer *>(module)) {
        (dynamic_cast<Analyzer *>(module))->analyze(event);
      }
    }
  } catch (AbortEventException &) {
    return false;
  }
  return true;
}

}  // namespace fire