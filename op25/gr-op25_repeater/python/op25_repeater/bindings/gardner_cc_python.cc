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
/* BINDTOOL_HEADER_FILE(gardner_cc.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(d1b0d443a6ec190eb29e50e99d90b6d3) */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/op25_repeater/gardner_cc.h>
// pydoc.h is automatically generated in the build directory
#include <gardner_cc_pydoc.h>

void bind_gardner_cc(py::module &m) {

  using gardner_cc = ::gr::op25_repeater::gardner_cc;

  py::class_<gardner_cc, gr::block, gr::basic_block,
             std::shared_ptr<gardner_cc>>(m, "gardner_cc", D(gardner_cc))

      .def(py::init(&gardner_cc::make), py::arg("samples_per_symbol"),
           py::arg("gain_mu"), py::arg("gain_omega"),
           py::arg("lock_threshold") = 0.28000000000000003, D(gardner_cc, make))

      .def("set_omega", &gardner_cc::set_omega, py::arg("omega"),
           D(gardner_cc, set_omega))

      .def("reset", &gardner_cc::reset, D(gardner_cc, reset))

      .def("locked", &gardner_cc::locked, D(gardner_cc, locked))

      .def("quality", &gardner_cc::quality, D(gardner_cc, quality))

      ;
}
