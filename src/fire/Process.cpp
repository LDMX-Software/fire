/**
 * @file Process.cxx
 * Implementation file for Process class
 */

#include "fire/Process.h"

#include <iostream>

#include "fire/Event.h"
#include "fire/EventFile.h"
#include "fire/EventProcessor.h"
#include "fire/Exception/Exception.h"
#include "fire/Logger.h"
#include "fire/NtupleManager.h"
#include "fire/RunHeader.h"
#include "fire/factory/Factory.hpp"

namespace fire {

Process::Process(const fire::config::Parameters &configuration)
    : conditions_{*this},
      output_file_{configuration.get<config::Parameters>("output_file")},
      input_files_{configuration.get<std::vector<std::string>>("input_files",{})},
      event_{configuration.get<std::string>("pass"),
             configuration.get<std::vector<config::Parameters>>("keep", {})},
      event_limit_{configuration.get<int>("event_limit")},
      log_frequency_{configuration.get<int>("log_frequency")},
      max_tries_{configuration.get<int>("max_tries")},
      run_{configuration.get<int>("run")},
      run_header_{nullptr}
      {
  logging::open(
      logging::convertLevel(configuration.get<int>("term_level")),
      logging::convertLevel(configuration.get<int>("file_level")),
      configuration.get<std::string>("log_file")
      );
  for (const auto &lib :
       configuration.get<std::vector<std::string>>("libraries", {}))
    factory::loadLibrary(lib);

  auto sequence{
      configuration.get<std::vector<config::Parameters>>("sequence", {})};
  if (sequence.empty() and not configuration.get<bool>("testing")) {
    EXCEPTION_RAISE(
        "NoSeq",
        "No sequence has been defined. What should I be doing?\nUse "
        "p.sequence to tell me what processors to run.");
  }
  for (const auto &proc : sequence) {
    auto class_name{proc.get<std::string>("class_name")};
    auto instance_name{proc.get<std::string>("instance_name")};
    std::unique_ptr<Processor> ep;
    try {
      ep = Processor::Factory::get().make(class_name, instance_name, *this);
    } catch (Exception const &e) {
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
  for (const auto &cop : conditionsObjectProviders) {
    auto class_name{cop.get<std::string>("class_name")};
    auto object_name{cop.get<std::string>("object_name")};
    auto tag_name{cop.get<std::string>("tag_name")};

    conditions_.createConditionsObjectProvider(class_name, object_name,
                                               tag_name, cop);
  }
}

void Process::run() {
  // counter for number of events we have processed
  std::size_t n_events_processed{0};
  // index of the entries in the output file
  //  equal to n_events_processed unless we are dropping events
  std::size_t i_output_file{0};

  // Start by notifying everyone that modules processing is beginning
  conditions_.onProcessStart();
  for (auto proc : sequence_) proc->onProcessStart();

  // If we have no input files, but do have an event number, run for
  // that number of events and generate an output file.
  if (input_files_.empty()) {
    for (auto module : sequence_) module->onFileOpen(output_file_);

    RunHeader run_header(run_);
    run_header.setRunStart(std::time(nullptr));
    newRun(run_header);

    for (; n_events_processed < event_limit_; n_events_processed++) {
      event_.header().setRun(run_);
      event_.header().setEventNumber(n_events_processed + 1);
      event_.header().setTimestamp(TTimeStamp());

      // keep trying to process this event until successful
      // or we hit the maximum number of tries
      for (int num_tries{0}; num_tries < max_tries_; num_tries++)
        if (process(n_events_processed,i_output_file)) break;
    }

    for (auto module : sequence_) module->onFileClose(outFile);

    run_header_->setRunEnd(std::time(nullptr));
    ldmx_log(info) << *run_header_;

    /**
     * TODO write run header to output file
     */
    try {
      h5::DataSet<RunHeader> write_ds{RunHeader::NAME,true,run_header_};
      write_ds.save(output_file_,0);
    } catch (.../*insert high five exception here*/) {
      // rethrow
    }

  } else {
    // there are input files
    
    /// the cache of runs from the opened input files
    std::unordered_map<int,RunHeader> input_runs;

    int ifile = 0;
    int wasRun = -1;
    for (auto fn : input_files_) {
      h5::Reader input_file{fn};

      /**
       * Load runs into in-memory cache
       */
      try {
        h5::DataSet<RunHeader> read_ds{RunHeader::NAME,false};
        // TODO write h5::DataSet<RunHeader>::load_all function for DataSet
        for (const auto& rh : read_ds.load_all(input_file)) {
          input_runs[rh.getRunNumber()] = rh;
        }
      } catch (.../*insert high five exception here*/) {
        // rethrow
      }

      ldmx_log(info) << "Opening " << input_file;

      for (auto module : sequence_) module->onFileOpen(input_file);
      event_.setInputFile(input_file);

      long unsigned int max_index = input_file.entries();
      if (max_index + n_events_processed > event_limit) 
        max_index = event_limit - n_events_processed;

      for (std::size_t i_entry_file{0}; i_entry_file < max_index; i_entry_file++) {
        // load data from input file
        event_.load(input_file, i_entry_file);

        // notify for new run if necessary
        if (event_.header().getRun() != wasRun) {
          wasRun = event_.header().getRun();
          if (input_runs.find(wasRun) != input_runs.end()) {
            newRun(input_runs[wasRun]);
            ldmx_log(info) << "Got new run header from '" << input_file
                           << "\n"
                           << runHeader();
          } else {
            ldmx_log(warn) << "Run header for run " << wasRun
                           << " was not found!";
          }
        }

        process(n_events_processed, i_output_file);

        n_events_processed++;
      }  // loop through events

      if (event_limit_ > 0 && n_events_processed == event_limit_) {
        ldmx_log(info) << "Reached event limit of " << event_limit_ << " events";
      }

      ldmx_log(info) << "Closing " << input_file;

      for (auto module : sequence_) module->onFileClose(input_file);
    }  // loop through input files

    // copy the input run headers to the output file
    try {
      h5::DataSet<RunHeader> write_ds(RunHeader::NAME,true);
      std::size_t i_run{0};
      for (const auto& [_,rh] : input_runs) {
        write_ds.update(rh);
        write_ds.save(output_file_, i_run++);  
      }
    } catch (.../*high five exception*/) {
    }
  }    // are there input files? if-else tree

  // finally, notify everyone that we are stopping
  for (auto module : sequence_) module->onProcessEnd();
}

void Process::newRun(RunHeader &rh) {
  // update pointer so asynchronous callers
  // can access the run header
  run_header_ = &rh;
  // Producers are allowed to put parameters into
  // the run header through 'beforeNewRun' method
  for (auto module : sequence_)
    if (dynamic_cast<Producer *>(module))
      dynamic_cast<Producer *>(module)->beforeNewRun(rh);
  // now run header has been modified by Producers,
  // it is valid to read from for everyone else in 'onNewRun'
  conditions_.onNewRun(rh);
  for (auto module : sequence_) module->onNewRun(rh);
}

bool Process::process(const std::size_t& n, std::size_t& i_output_file) {
  // status statement printed to log
  if ((logFrequency_ != -1) && ((n + 1) % logFrequency_ == 0)) {
    TTimeStamp t; // TODO replace TTimeStamp
    ldmx_log(info) << "Processing " << n + 1 << " Run "
                   << event_.header().getRun() << " Event "
                   << event_.header().getEventNumber() << "  ("
                   << t.AsString("lc") << ")";
  }

  try {
    // new event processing, forget old information
    storage_controller_.resetEventState();

    // go through each processor in the sequence in order
    for (auto proc : sequence_) {
      if (dynamic_cast<Producer *>(proc)) {
        (dynamic_cast<Producer *>(proc))->produce(event_);
      } else if (dynamic_cast<Analyzer *>(proc)) {
        (dynamic_cast<Analyzer *>(proc))->analyze(event_);
      }
    }
  } catch (AbortEventException &) {
    return false;
  }

  // we didn't abort the event, so we should give the option to save it
  if (storage_controller_.keepEvent()) {
    event_.checkThenSave(output_file_, i_output_file++);
  }

  return true;
}

}  // namespace fire
