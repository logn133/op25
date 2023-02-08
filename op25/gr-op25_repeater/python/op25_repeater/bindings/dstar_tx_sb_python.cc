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
/* BINDTOOL_HEADER_FILE(dstar_tx_sb.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(15a53983d26e3470e692a851c0bb76bb) */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/op25_repeater/dstar_tx_sb.h>
// pydoc.h is automatically generated in the build directory
#include <dstar_tx_sb_pydoc.h>

void bind_dstar_tx_sb(py::module &m) {

  using dstar_tx_sb = ::gr::op25_repeater::dstar_tx_sb;

  py::class_<dstar_tx_sb, gr::block, gr::basic_block,
             std::shared_ptr<dstar_tx_sb>>(m, "dstar_tx_sb", D(dstar_tx_sb))

      .def(py::init(&dstar_tx_sb::make), py::arg("versbose_flag"),
           py::arg("conf_file"), D(dstar_tx_sb, make))

      .def("set_gain_adjust", &dstar_tx_sb::set_gain_adjust,
           py::arg("gain_adjust"), D(dstar_tx_sb, set_gain_adjust))

      ;
}
