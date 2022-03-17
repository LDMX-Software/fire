#include "fire/Process.h"

#include <iostream>

#include "fire/factory/Factory.h"

namespace fire {

/**
 * open an input file for reading
 *
 * @note We determine the type of input file to open from
 * the extension of the file name. We could implement a
 * "header reading" but that is harder to develop compared
 * to a relatively simple requirement that almost everyone
 * already follows.
 *
 * @throws fire::Exception if the file does not have a
 * recognized extension.
 * @see fire::factory::Factory::make for how the readers
 * are constructed
 *
 * @param[in] fp file path to file to open
 * @return pointer to io::Reader that has opened file
 */
static std::unique_ptr<io::Reader> open(const std::string& fp) {
  static const std::map<std::string, std::string> ext_to_type = {
    { "root", "fire::io::root::Reader" },
    { "hdf5", "fire::io::h5::Reader" },
    { "h5"  , "fire::io::h5::Reader" }
  };
  auto ext{fp.substr(fp.find_last_of('.')+1)};
  try {
    return io::Reader::Factory::get().make(ext_to_type.at(ext), fp);
  } catch (const std::out_of_range&) {
    throw Exception("BadExt",
        "Unrecognized extension '"+ext+"' for input file "+fp+".");
  }
}

Process::Process(const fire::config::Parameters& configuration)
    : output_file_{configuration.get<int>("event_limit"),
                   configuration.get<config::Parameters>("output_file")},
      input_files_{
          configuration.get<std::vector<std::string>>("input_files", {})},
      event_{output_file_,
             configuration.get<std::string>("pass_name"),
             configuration.get<std::vector<config::Parameters>>("drop_keep_rules", {})},
      event_limit_{configuration.get<int>("event_limit")},
      log_frequency_{configuration.get<int>("log_frequency")},
      max_tries_{configuration.get<int>("max_tries")},
      run_{configuration.get<int>("run")},
      storage_control_{configuration.get<config::Parameters>("storage")},
      run_header_{nullptr} {
  logging::open(logging::convertLevel(configuration.get<int>("term_level", 4)),
                logging::convertLevel(configuration.get<int>("file_level", 4)),
                configuration.get<std::string>("log_file", ""));

  // load the libraries of ConditionsProviders and Processors
  for (const auto& lib :
       configuration.get<std::vector<std::string>>("libraries", {}))
    factory::loadLibrary(lib);

  // construct conditions system and the registered providers
  conditions_ = std::make_unique<Conditions>(
      configuration.get<config::Parameters>("conditions"), *this);

  auto sequence{
      configuration.get<std::vector<config::Parameters>>("sequence", {})};
  if (sequence.empty() and not configuration.get<bool>("testing",false)) {
    throw Exception("Config",
        "No sequence has been defined. What should I be doing?\nUse "
        "p.sequence to tell me what processors to run.",false);
  }
  for (const auto& proc : sequence) {
    auto class_name{proc.get<std::string>("class_name")};
    sequence_.emplace_back(Processor::Factory::get().make(class_name, proc, *this));
  }
}

Process::~Process() { logging::close(); }

void Process::run() {
  // counter for number of events we have processed
  std::size_t n_events_processed{0};

  // Start by notifying everyone that modules processing is beginning
  conditions_->onProcessStart();
  for (auto& proc : sequence_) proc->onProcessStart();

  // If we have no input files, but do have an event number, run for
  // that number of events and generate an output file.
  if (input_files_.empty()) {
    fire_log(info) << "No input files, starting production mode run.";

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
        if (process(n_events_processed)) break;
    }

    runHeader().runEnd();
    fire_log(info) << runHeader();

    try {
      io::Data<RunHeader> write_d{RunHeader::NAME, run_header_};
      write_d.save(output_file_);
    } catch (const HighFive::Exception&) {
      throw Exception("RunWrite","Unable to write RunHeader to output file "
          +output_file_.name());
    }

  } else {
    fire_log(info) << input_files_.size()
                   << " input file(s), starting reconstruction mode run.";
    // there are input files

    /// the cache of runs from the opened input files
    std::unordered_map<int, RunHeader> input_runs;

    int ifile = 0;
    int wasRun = -1;
    for (auto fn : input_files_) {
      std::unique_ptr<io::Reader> input_file = open(fn);

      /**
       * Load runs into in-memory cache
       */
      try {
        io::Data<RunHeader> read_d{RunHeader::NAME};
        std::size_t num_runs = input_file->runs();
        for (std::size_t i_run{0}; i_run < num_runs; i_run++) {
          input_file->load_into(read_d);
          // deep copy
          input_runs[read_d.get().getRunNumber()] = read_d.get();
        }
      } catch (const HighFive::Exception&) {
        throw Exception("RunRead","Unable to extract runs from input file "+input_file->name());
      }

      fire_log(info) << "Opening " << input_file->name();

      for (auto& module : sequence_) module->onFileOpen(input_file->name());
      event_.setInputFile(input_file.get());

      long unsigned int max_index = input_file->entries();
      if (event_limit_ > 0 and max_index + n_events_processed > event_limit_)
        max_index = event_limit_ - n_events_processed;

      for (std::size_t i_entry_file{0}; i_entry_file < max_index;
           i_entry_file++) {
        // load data from input file
        input_file->next(); // for ROOT reader to increment TTree entry
        event_.load(); // pull data from disk into memory

        // notify for new run if necessary
        if (event_.header().getRun() != wasRun) {
          wasRun = event_.header().getRun();
          if (input_runs.find(wasRun) != input_runs.end()) {
            newRun(input_runs[wasRun]);
            fire_log(info) << "Got new run header from '" << input_file->name() << "\n"
                           << runHeader();
          } else {
            fire_log(warn) << "Run header for run " << wasRun
                           << " was not found!";
          }
        }

        process(n_events_processed);

        n_events_processed++;
      }  // loop through events

      if (event_limit_ > 0 && n_events_processed == event_limit_) {
        fire_log(info) << "Reached event limit of " << event_limit_
                       << " events";
      }

      fire_log(info) << "Closing " << input_file->name();

      for (auto& proc : sequence_) proc->onFileClose(input_file->name());
    }  // loop through input files

    // copy the input run headers to the output file
    try {
      io::Data<RunHeader> write_d(RunHeader::NAME);
      for (const auto& [_, rh] : input_runs) {
        write_d.update(rh);
        write_d.save(output_file_);
      }
    } catch (const HighFive::Exception& ) {
      throw Exception("RunWrite","Unable to write run headers to output file "
          +output_file_.name());
    }
  }  // are there input files? if-else tree

  // allow event bus to put final touches into the output file
  event_.done();
  // finally, notify everyone that we are stopping
  for (auto& proc : sequence_) proc->onProcessEnd();
  conditions_->onProcessEnd();
}

void Process::newRun(RunHeader& rh) {
  // update pointer so asynchronous callers
  // can access the run header via the Process
  run_header_ = &rh;
  // Processors are allowed to put parameters into
  // the run header through 'beforeNewRun' method
  for (auto& proc : sequence_) proc->beforeNewRun(rh);
  // now run header has been modified by Processors,
  // it is valid to read from for everyone else in 'onNewRun'
  conditions_->onNewRun(rh);
  for (auto& proc : sequence_) proc->onNewRun(rh);
}

bool Process::process(const std::size_t& n) {
  // status statement printed to log
  if ((log_frequency_ != -1) && ((n + 1) % log_frequency_ == 0)) {
    std::time_t time = std::time(nullptr);
    fire_log(info) << "Processing " << n + 1 << " Run "
                   << event_.header().getRun() << " Event "
                   << event_.header().getEventNumber() << " : "
                   << std::asctime(std::localtime(&time));
  }

  // new event processing, forget old information
  storage_control_.resetEventState();

  try {
    // go through each processor in the sequence in order
    for (auto& proc : sequence_) proc->process(event_);
  } catch (Processor::AbortEventException&) {
    return false;
  }

  // we didn't abort the event, so we should give the option to save it
  if (storage_control_.keepEvent()) {
    event_.save();
  }

  // move to the next event
  event_.next();

  return true;
}

}  // namespace fire
