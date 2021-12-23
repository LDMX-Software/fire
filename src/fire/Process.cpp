#include "fire/Process.hpp"

#include <iostream>

#include "fire/factory/Factory.hpp"

namespace fire {

Process::Process(const fire::config::Parameters &configuration)
    : conditions_{configuration.get<config::Parameters>("conditions"),*this},
      output_file_{configuration.get<config::Parameters>("output_file")},
      input_files_{configuration.get<std::vector<std::string>>("input_files",{})},
      event_{configuration.get<std::string>("pass"),
             configuration.get<std::vector<config::Parameters>>("keep", {})},
      event_limit_{configuration.get<int>("event_limit")},
      log_frequency_{configuration.get<int>("log_frequency")},
      max_tries_{configuration.get<int>("max_tries")},
      run_{configuration.get<int>("run")},
      storage_control_{configuration.get<config::Parameters>("storage")},
      run_header_{nullptr}
      {
  logging::open(
      logging::convertLevel(configuration.get<int>("term_level",4)),
      logging::convertLevel(configuration.get<int>("file_level",4)),
      configuration.get<std::string>("log_file","")
      );

  for (const auto &lib :
       configuration.get<std::vector<std::string>>("libraries", {}))
    factory::loadLibrary(lib);

  auto sequence{
      configuration.get<std::vector<config::Parameters>>("sequence", {})};
  if (sequence.empty() and not configuration.get<bool>("testing")) {
    throw fire::config::Parameters::Exception(
        "No sequence has been defined. What should I be doing?\nUse "
        "p.sequence to tell me what processors to run.");
  }
  for (const auto &proc : sequence) {
    auto class_name{proc.get<std::string>("class_name")};
    sequence_.emplace_back(Processor::Factory::get().make(class_name, proc));
    sequence_.back()->attach(this);
  }
}

Process::~Process() {
  logging::close();
}

void Process::run() {
  // counter for number of events we have processed
  std::size_t n_events_processed{0};
  // index of the entries in the output file
  //  equal to n_events_processed unless we are dropping events
  std::size_t i_output_file{0};

  // Start by notifying everyone that modules processing is beginning
  conditions_.onProcessStart();
  for (auto& proc : sequence_) proc->onProcessStart();

  // If we have no input files, but do have an event number, run for
  // that number of events and generate an output file.
  if (input_files_.empty()) {
    fire_log(info) << "No input files, starting production mode run.";
    for (auto& proc : sequence_) proc->onFileOpen(output_file_.name());

    RunHeader run_header;
    run_header.runStart(run_);
    newRun(run_header);

    for (; n_events_processed < event_limit_; n_events_processed++) {
      event_.header().setRun(run_);
      event_.header().setEventNumber(n_events_processed + 1);
      event_.header().setTimestamp();

      // keep trying to process this event until successful
      // or we hit the maximum number of tries
      for (int num_tries{0}; num_tries < max_tries_; num_tries++)
        if (process(n_events_processed,i_output_file)) break;
    }

    for (auto& proc : sequence_) proc->onFileClose(output_file_.name());

    runHeader().runEnd();
    fire_log(info) << runHeader();

    h5::DataSet<RunHeader> write_ds{RunHeader::NAME,true,run_header_};
    write_ds.save(output_file_,0);

  } else {
    fire_log(info) << input_files_.size() << " input file(s), starting reconstruction mode run.";
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
      {
        h5::DataSet<RunHeader> read_ds{RunHeader::NAME,false};
        std::size_t num_runs = input_file.runs();
        for (std::size_t i_run{0}; i_run < num_runs; i_run++) {
          read_ds.load(input_file, i_run);
          // deep copy
          input_runs[read_ds.get().getRunNumber()] = read_ds.get();
        }
      }

      fire_log(info) << "Opening " << input_file;

      for (auto &module : sequence_) module->onFileOpen(input_file.name());
      event_.setInputFile(input_file);

      long unsigned int max_index = input_file.entries();
      if (event_limit_ > 0 and max_index + n_events_processed > event_limit_) 
        max_index = event_limit_ - n_events_processed;

      for (std::size_t i_entry_file{0}; i_entry_file < max_index; i_entry_file++) {
        // load data from input file
        event_.load(input_file, i_entry_file);

        // notify for new run if necessary
        if (event_.header().getRun() != wasRun) {
          wasRun = event_.header().getRun();
          if (input_runs.find(wasRun) != input_runs.end()) {
            newRun(input_runs[wasRun]);
            fire_log(info) << "Got new run header from '" << input_file
                           << "\n"
                           << runHeader();
          } else {
            fire_log(warn) << "Run header for run " << wasRun
                           << " was not found!";
          }
        }

        process(n_events_processed, i_output_file);

        n_events_processed++;
      }  // loop through events

      if (event_limit_ > 0 && n_events_processed == event_limit_) {
        fire_log(info) << "Reached event limit of " << event_limit_ << " events";
      }

      fire_log(info) << "Closing " << input_file;

      for (auto& proc : sequence_) proc->onFileClose(input_file.name());
    }  // loop through input files

    // copy the input run headers to the output file
    {
      h5::DataSet<RunHeader> write_ds(RunHeader::NAME,true);
      std::size_t i_run{0};
      for (const auto& [_,rh] : input_runs) {
        write_ds.update(rh);
        write_ds.save(output_file_, i_run++);  
      }
    }
  }    // are there input files? if-else tree

  // finally, notify everyone that we are stopping
  for (auto& proc : sequence_) proc->onProcessEnd();
}

void Process::newRun(RunHeader &rh) {
  // update pointer so asynchronous callers
  // can access the run header via the Process
  run_header_ = &rh;
  // Producers are allowed to put parameters into
  // the run header through 'beforeNewRun' method
  for (auto& proc : sequence_) proc->beforeNewRun(rh);
  // now run header has been modified by Producers,
  // it is valid to read from for everyone else in 'onNewRun'
  conditions_.onNewRun(rh);
  for (auto& proc : sequence_) proc->onNewRun(rh);
}

bool Process::process(const std::size_t& n, std::size_t& i_output_file) {
  // status statement printed to log
  if ((log_frequency_ != -1) && ((n + 1) % log_frequency_ == 0)) {
    std::time_t time = std::time(nullptr);
    fire_log(info) << "Processing " << n + 1 << " Run "
                   << event_.header().getRun() << " Event "
                   << event_.header().getEventNumber()
                   << " : " << std::asctime(std::localtime(&time));
                   
  }

  // new event processing, forget old information
  storage_control_.resetEventState();

  try {
    // go through each processor in the sequence in order
    for (auto& proc : sequence_) proc->process(event_);
  } catch (Processor::AbortEventException& ) {
    return false;
  }

  // we didn't abort the event, so we should give the option to save it
  if (storage_control_.keepEvent()) {
    event_.save(output_file_, i_output_file++);
  }

  // move to the next event
  event_.next();

  return true;
}

}  // namespace fire
