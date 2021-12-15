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
    : conditions_{*this},
  output_file_{configuration.get<config::Parameters>("output_file")},
  pass_{configuration.get<std::string>("pass")},
  event_{configuration.get<std::string>("pass"),configuration.get<std::vector<config::Parameters>>("keep",{})},
  event_limit_{configuration.get<int>("event_limit")},
  log_frequency_{configuration.get<int>("log_frequency")},
  term_level_{configuration.get<int>("term_level")},
  max_tries_{configuration.get<int>("max_tries")},
  run_{configuration.get<int>("run")} {

  for (const auto &input_file : configuration.get<std::vector<config::Parameters>>("input_files",{}))
    input_files_.emplace_back(input_file);

  for (const auto &lib : configuration.get<std::vector<std::string>>("libraries",{}))
    factory::loadLibrary(lib);

  auto sequence{configuration.get<std::vector<config::Parameters>>( "sequence", {})};
  if (sequence.empty() and not configuration.get<bool>("testing")) {
    EXCEPTION_RAISE(
        "NoSeq",
        "No sequence has been defined. What should I be doing?\nUse "
        "p.sequence to tell me what processors to run.");
  }
  for (const auto& proc : sequence) {
    auto class_name{proc.get<std::string>("class_name")};
    auto instance_name{proc.get<std::string>("instance_name")};
    std::unique_ptr<Processor> ep;
    try {
      ep = Processor::Factory::get().make(class_name, instance_name, *this);
    } catch (Exception const& e) {
      EXCEPTION_RAISE(
          "UnableToCreate",
          "Unable to create instance '" + instance_name + "' of class '" +
              class_name +
              "'. Did you load the library that this class is apart of?");
    }
    ep->configure(proc);
    sequence_.push_back(ep);
  }

  auto conditionsObjectProviders{
      configuration.getParameter<std::vector<fire::config::Parameters>>(
          "conditionsObjectProviders", {})};
  for (const auto& cop : conditionsObjectProviders) {
    auto class_name{cop.get<std::string>("class_name")};
    auto object_name{cop.get<std::string>("object_name")};
    auto tag_name{cop.get<std::string>("tag_name")};

    conditions_.createConditionsObjectProvider(class_name, object_name, tag_name,
                                               cop);
  }
}

void Process::run() {
  // set up the logging for this run
  logging::open(logging::convertLevel(term_level_));

  // Counter to keep track of the number of events that have been
  // procesed
  auto n_events_processed{0};

  // Start by notifying everyone that modules processing is beginning
  conditions_.onProcessStart();
  for (auto proc : sequence_) proc->onProcessStart();

  // If we have no input files, but do have an event number, run for
  // that number of events and generate an output file.
  if (input_files_.empty() && eventLimit_ > 0) {
    for (auto module : sequence_) module->onFileOpen(output_file_);

    ldmx::RunHeader runHeader(run_);
    runHeader.setRunStart(std::time(nullptr));  // set run starting
    runHeader_ = &runHeader;            // give handle to run header to process
    output_file_.writeRunHeader(runHeader);  // add run header to file

    newRun(runHeader);

    int numTries = 0;  // number of tries for the current event number
    while (n_events_processed < event_limit_) {
      event_.header().setRun(run_);
      event_.header().setEventNumber(n_events_processed + 1);
      event_.header().setTimestamp(TTimeStamp());

      numTries++;

      // reset the storage controller state
      storageController_.resetEventState();

      bool completed = process(n_events_processed,event_);

      outFile.nextEvent(storageController_.keepEvent(completed));

      if (completed or numTries >= max_tries_) {
        n_events_processed++;                 // increment events made
        numTries = 0;                         // reset try counter
      }
    }

    for (auto module : sequence_) module->onFileClose(outFile);

    runHeader.setRunEnd(std::time(nullptr));
    ldmx_log(info) << runHeader;
    outFile.close();

  } else {
    // there are input files

    EventFile *outFile(0);

    bool singleOutput = false;
    if (outputFiles_.size() == 1) {
      singleOutput = true;
    } else if (!outputFiles_.empty() and
               outputFiles_.size() != inputFiles_.size()) {
      EXCEPTION_RAISE("Process",
                      "Unable to handle case of different number of input and "
                      "output files (other than zero/one ouput file).");
    }

    // next, loop through the files
    int ifile = 0;
    int wasRun = -1;
    for (auto infilename : inputFiles_) {
      EventFile inFile(config_, infilename);

      ldmx_log(info) << "Opening file " << infilename;

      for (auto module : sequence_) module->onFileOpen(inFile);

      // configure event file that will be iterated over
      EventFile *masterFile;
      if (!outputFiles_.empty()) {
        // setup new output file if either
        // 1) we are not in single output mode
        // 2) this is the first input file
        if (!singleOutput or ifile == 0) {
          // setup new output file
          outFile = new EventFile(config_, outputFiles_[ifile], &inFile, singleOutput);
          ifile++;

          // setup theEvent we will iterate over
          if (outFile) {
            outFile->setupEvent(&theEvent);
            masterFile = outFile;
          } else {
            EXCEPTION_RAISE("Process", "Unable to construct output file for " +
                                           outputFiles_[ifile]);
          }

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

TDirectory *Process::makeHistoDirectory(const std::string &dirName) {
  auto owner{openHistoFile()};
  TDirectory *child = owner->mkdir((char *)dirName.c_str());
  if (child) child->cd();
  return child;
}

TDirectory *Process::openHistoFile() {
  TDirectory *owner{nullptr};

  if (histoFilename_.empty()) {
    // trying to write histograms/ntuples but no file defined
    EXCEPTION_RAISE("NoHistFileName",
                    "You did not provide the necessary histogram file name to "
                    "put your histograms (or ntuples) in.\n    Provide this "
                    "name in the python configuration with 'p.histogramFile = "
                    "\"myHistFile.root\"' where p is the Process object.");
  } else if (histoTFile_ == nullptr) {
    histoTFile_ = new TFile(histoFilename_.c_str(), "RECREATE");
    owner = histoTFile_;
  } else
    owner = histoTFile_;
  owner->cd();

  return owner;
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
