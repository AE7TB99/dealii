// ---------------------------------------------------------------------
//
// Copyright (C) 1998 - 2018 by the deal.II authors
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



#include <deal.II/base/function.h>

#include <deal.II/distributed/solution_transfer.h>
#include <deal.II/distributed/tria.h>

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_dgq.h>

#include <deal.II/grid/grid_generator.h>

#include <deal.II/hp/dof_handler.h>
#include <deal.II/hp/fe_collection.h>

#include <deal.II/lac/la_parallel_vector.h>

#include <deal.II/numerics/vector_tools.h>

#include <iostream>
#include <vector>

#include "../tests.h"


template <int dim>
void
transfer(std::ostream &out)
{
  parallel::distributed::Triangulation<dim> tria(MPI_COMM_WORLD);
  GridGenerator::hyper_cube(tria);
  tria.refine_global(2);

  const unsigned int    max_degree = 6 - dim;
  hp::FECollection<dim> fe_dgq;
  for (unsigned int deg = 1; deg <= max_degree; ++deg)
    fe_dgq.push_back(FE_Q<dim>(deg));

  hp::DoFHandler<dim> dgq_dof_handler(tria);

  // randomly assign FE orders
  for (const auto &cell : dgq_dof_handler.active_cell_iterators())
    if (cell->is_locally_owned())
      cell->set_active_fe_index(Testing::rand() % max_degree);
  dgq_dof_handler.distribute_dofs(fe_dgq);

  IndexSet dgq_locally_owned_dofs = dgq_dof_handler.locally_owned_dofs();
  IndexSet dgq_locally_relevant_dofs;
  dealii::DoFTools::extract_locally_relevant_dofs(dgq_dof_handler,
                                                  dgq_locally_relevant_dofs);
  IndexSet dgq_ghost_dofs = dgq_locally_relevant_dofs;
  dgq_ghost_dofs.subtract_set(dgq_locally_owned_dofs);

  LinearAlgebra::distributed::Vector<double> dgq_solution;
  dgq_solution.reinit(dgq_locally_owned_dofs, dgq_ghost_dofs, MPI_COMM_WORLD);

  VectorTools::interpolate(dgq_dof_handler, ZeroFunction<dim>(), dgq_solution);

  parallel::distributed::SolutionTransfer<
    dim,
    LinearAlgebra::distributed::Vector<double>,
    hp::DoFHandler<dim>>
    dgq_soltrans(dgq_dof_handler);

  LinearAlgebra::distributed::Vector<double> dgq_old_solution = dgq_solution;
  dgq_old_solution.update_ghost_values();
  {
    unsigned int counter = 0;
    for (auto cell = tria.begin_active(); cell != tria.end(); ++cell, ++counter)
      if (cell->is_locally_owned())
        {
          if (counter > ((dim == 2) ? 4 : 8))
            cell->set_coarsen_flag();
          else
            cell->set_refine_flag();
        }
  }

  tria.prepare_coarsening_and_refinement();
  dgq_soltrans.prepare_for_coarsening_and_refinement(dgq_old_solution);
  tria.execute_coarsening_and_refinement();

  dgq_dof_handler.distribute_dofs(fe_dgq);

  dgq_locally_owned_dofs = dgq_dof_handler.locally_owned_dofs();
  dealii::DoFTools::extract_locally_relevant_dofs(dgq_dof_handler,
                                                  dgq_locally_relevant_dofs);
  dgq_ghost_dofs = dgq_locally_relevant_dofs;
  dgq_ghost_dofs.subtract_set(dgq_locally_owned_dofs);

  dgq_solution.reinit(dgq_locally_owned_dofs, dgq_ghost_dofs, MPI_COMM_WORLD);
  dgq_soltrans.interpolate(dgq_solution);

  deallog << "OK" << std::endl;
}


int
main(int argc, char *argv[])
{
  Utilities::MPI::MPI_InitFinalize mpi_initialization(argc, argv, 1);
  MPILogInitAll                    log;

  deallog.push("2d");
  transfer<2>(deallog.get_file_stream());
  deallog.pop();
  deallog.push("3d");
  transfer<3>(deallog.get_file_stream());
  deallog.pop();
}
