/**
 * @file Open.h
 * Helper function for opening files of various type
 */

#ifndef FIRE_IO_OPEN_H
#define FIRE_IO_OPEN_H

#include "fire/io/Reader.h"

namespace fire::io {

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
 * ## Recognized Extensions
 * - `root` : use fire::io::root::Reader
 * - `h5` or `hdf5` : use fire::io::h5::Reader
 *
 * @param[in] fp file path to file to open
 * @return pointer to io::Reader that has opened file
 */
std::unique_ptr<io::Reader> open(const std::string& fp);

}

#endif
