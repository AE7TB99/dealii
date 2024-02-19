// ---------------------------------------------------------------------
//
// Copyright (C) 2018 - 2020 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE.md at
// the top level directory of deal.II.
//
// ---------------------------------------------------------------------

// A test that checks make_flux_sparsity_pattern
// with a FECollection containing FE_Q and FE_Nothing elements.

//   x__________x__________ _________
//   |          |          |         |
//   |    FE_Q  |   FE_N   |   FE_N  |     x: constrained DoFs
//   |          |          |         |
//   |__________|__________|_________|
//   x          x

#include <deal.II/distributed/tria.h>

#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_nothing.h>
#include <deal.II/fe/fe_q.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_refinement.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/grid/tria_iterator.h>

#include <deal.II/hp/fe_collection.h>

#include <deal.II/lac/affine_constraints.h>
#include <deal.II/lac/trilinos_sparse_matrix.h>

#include <iostream>
#include <vector>

#include "../tests.h"

template <int dim>
void
check()
{
  // create triangulation
  Triangulation<dim>        triangulation;
  std::vector<unsigned int> subdivisions(dim, 1U);
  subdivisions[0] = 3;
  subdivisions[1] = 1;
  if (dim == 3)
    subdivisions[2] = 1;
  Point<dim> p1, p2;
  switch (dim)
    {
      case 2:
        p1[0] = p1[1] = 0.0;
        p2[0]         = 3.0;
        p2[1]         = 1.0;
        break;
      case 3:
        p1[0] = p1[1] = p1[2] = 0.0;
        p2[0]                 = 3.0;
        p2[1] = p2[2] = 1.0;
        break;
      default:
        DEAL_II_NOT_IMPLEMENTED();
    }

  GridGenerator::subdivided_hyper_rectangle(triangulation,
                                            subdivisions,
                                            p1,
                                            p2);

  // create FE Collection and insert two FE objects
  // Q1 and one FE object Nothing
  hp::FECollection<dim> fe_collection;

  fe_collection.push_back(FE_Q<dim>(1));
  fe_collection.push_back(FE_Nothing<dim>());

  // set-up DoFHandler
  DoFHandler<dim> dof_handler(triangulation);

  // set active fe index
  unsigned int i = 0;

  for (const auto &cell : dof_handler.active_cell_iterators())
    {
      if (i == 2)
        {
          cell->set_active_fe_index(0);
          ++i;
        }
      else
        {
          cell->set_active_fe_index(1);
          ++i;
        }
    }

  dof_handler.distribute_dofs(fe_collection);

  {
    // set constraints
    AffineConstraints<double> constraints;

    for (unsigned i = 0; i < Utilities::pow<unsigned int>(2, dim); ++i)
      constraints.add_constraint(i, {}, i);

    constraints.close();

    // setup sparsity pattern
    DynamicSparsityPattern dsp(dof_handler.n_dofs(), dof_handler.n_dofs());
    DoFTools::make_flux_sparsity_pattern(dof_handler, dsp, constraints);
    dsp.compress();

    // print sparsity pattern
    deallog << "nonzero matrix elements: " << dsp.n_nonzero_elements()
            << std::endl;
    dsp.print(deallog.get_file_stream());
  }
}



int
main()
{
  initlog();

  deallog.push("2d");
  check<2>();
  deallog.pop();
  deallog.push("3d");
  check<3>();
  deallog.pop();
}
