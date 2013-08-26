// Copyright (C) 2010-2013 von Karman Institute for Fluid Dynamics, Belgium
//
// This software is distributed under the terms of the
// GNU Lesser General Public License version 3 (LGPLv3).
// See doc/lgpl.txt and doc/gpl.txt for the license text.

#include "common/Builder.hpp"

#include "mesh/ShapeFunctionT.hpp"
#include "mesh/LagrangeP0/LibLagrangeP0.hpp"
#include "mesh/LagrangeP0/Quad.hpp"

namespace cf3 {
namespace mesh {
namespace LagrangeP0 {

////////////////////////////////////////////////////////////////////////////////

common::ComponentBuilder < ShapeFunctionT<Quad>, ShapeFunction, LibLagrangeP0 >
   Quad_Builder(LibLagrangeP0::library_namespace()+"."+Quad::type_name());

////////////////////////////////////////////////////////////////////////////////

void Quad::compute_value(const MappedCoordsT& mapped_coord, ValueT& result)
{
  result[0] = 1.;
}

////////////////////////////////////////////////////////////////////////////////

void Quad::compute_gradient(const MappedCoordsT& mapped_coord, GradientT& result)
{
  result(KSI,0) = 0.;
  result(ETA,0) = 0.;
}

////////////////////////////////////////////////////////////////////////////////

const RealMatrix& Quad::local_coordinates()
{
  static const RealMatrix loc_coord =
      (RealMatrix(nb_nodes, dimensionality) <<

       0., 0.

       ).finished();
  return loc_coord;
}

////////////////////////////////////////////////////////////////////////////////

const RealMatrix& Quad::mononomial_coefficients()
{
  static const RealMatrix coeffs =
      (RealMatrix(nb_nodes, nb_nodes) <<

       1.

       ).finished();
  return coeffs;
}

////////////////////////////////////////////////////////////////////////////////

const RealMatrix& Quad::mononomial_exponents()
{
  static const RealMatrix exponents =
      (RealMatrix(nb_nodes, dimensionality) <<

       0, 0

       ).finished();
  return exponents;
}

////////////////////////////////////////////////////////////////////////////////

} // LagrangeP0
} // mesh
} // cf3
