/*
 * Copyright 2023 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually
 * edited  */
/* The following lines can be configured to regenerate this file during cmake */
/* If manual edits are made, the following tags should be modified accordingly.
 */
/* BINDTOOL_GEN_AUTOMATIC(0) */
/* BINDTOOL_USE_PYGCCXML(0) */
/* BINDTOOL_HEADER_FILE(dmr_bs_tx_bb.h) */
/* BINDTOOL_HEADER_FILE_HASH(a53b7aea26b87ad3ab2eb7cfc0862732) */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/op25_repeater/dmr_bs_tx_bb.h>
// pydoc.h is automatically generated in the build directory
#include <dmr_bs_tx_bb_pydoc.h>

void bind_dmr_bs_tx_bb(py::module &m) {

  using dmr_bs_tx_bb = ::gr::op25_repeater::dmr_bs_tx_bb;

  py::class_<dmr_bs_tx_bb, gr::block, gr::basic_block,
             std::shared_ptr<dmr_bs_tx_bb>>(m, "dmr_bs_tx_bb", D(dmr_bs_tx_bb))

      .def(py::init(&dmr_bs_tx_bb::make), py::arg("versbose_flag"),
           py::arg("conf_file"), D(dmr_bs_tx_bb, make))

      ;
}
