/*            Ralf Hartmann, University of Heidelberg, Dez 98               */


#include <fe/fe_lib.dg.h>
#include <grid/tria_iterator.h>
#include <grid/dof_accessor.h>
#include <grid/geometry_info.h>
#include <algorithm>




#if deal_II_dimension == 1

template <>
FEDG_Q0<1>::FEDG_Q0 () :
		FEQ1Mapping<1> (0, 1)
{
				   // for restriction and prolongation matrices:
				   // note that we do not add up all the
				   // contributions since then we would get
				   // two summands per vertex in 1d (four
				   // in 2d, etc), but only one per line dof.
				   // We could accomplish for that by dividing
				   // the vertex dof values by 2 (4, etc), but
				   // would get into trouble at the boundary
				   // of the domain since there only one
				   // cell contributes to a vertex. Rather,
				   // we do not add up the contributions but
				   // set them right into the matrices!
				   
				   // The restriction matrices got crazy values
				   // as it is yet not clear how they should work
				   // in the DG(0) case. In general
				   // the use of the restriction matrices
				   // is not yet finally decided about, too.
  restriction[0](0,0) = 1e8;
  restriction[1](0,0) = 1e8;

  prolongation[0](0,0) = 1.0;
  prolongation[1](0,0) = 1.0;
};



template <>
void
FEDG_Q0<1>::get_face_support_points (const DoFHandler<1>::face_iterator &,
				     vector<Point<1> >  &) const {
  Assert (false, ExcInternalError());
};

#endif




#if deal_II_dimension == 2

template <>
FEDG_Q0<2>::FEDG_Q0 () :
		FEQ1Mapping<2> (0, 0, 1)
{
				   // The restriction matrices got crazy values
				   // as it is yet not clear how they should work
				   // in the DG(0) case. In general
				   // the use of the restriction matrices
				   // is not yet finally decided about, too.
  restriction[0](0,0) = 1e8;
  restriction[1](0,0) = 1e8;
  restriction[2](0,0) = 1e8;
  restriction[3](0,0) = 1e8;

  prolongation[0](0,0) = 1.0;

  prolongation[1](0,0) = 1.0;

  prolongation[2](0,0) = 1.0;

  prolongation[3](0,0) = 1.0;
};



#endif




template <int dim>
inline
double
FEDG_Q0<dim>::shape_value (const unsigned int i,
				const Point<dim>&) const
{
  Assert((i<total_dofs), ExcIndexRange(i, 0, total_dofs));
  return 1.;
};



template <int dim>
inline
Tensor<1,dim>
FEDG_Q0<dim>::shape_grad (const unsigned int i,
			       const Point<dim>&) const
{
  Assert((i<total_dofs), ExcIndexRange(i, 0, total_dofs));
  return Tensor<1,dim> ();
};



template <int dim>
inline
Tensor<2,dim>
FEDG_Q0<dim>::shape_grad_grad (const unsigned int i,
				  const Point<dim> &) const
{
  Assert((i<total_dofs), ExcIndexRange(i, 0, total_dofs));

  return Tensor<2,dim>();
};



template <int dim>
void FEDG_Q0<dim>::get_local_mass_matrix (const DoFHandler<dim>::cell_iterator &cell,
					       FullMatrix<double> &local_mass_matrix) const {
  Assert (local_mass_matrix.n() == total_dofs,
	  ExcWrongFieldDimension(local_mass_matrix.n(),total_dofs));
  Assert (local_mass_matrix.m() == total_dofs,
	  ExcWrongFieldDimension(local_mass_matrix.m(),total_dofs));

  local_mass_matrix(0,0) = cell->measure();
};



template <int dim>
void
FEDG_Q0<dim>::get_unit_support_points (vector<Point<dim> > &unit_points) const {
  Assert (unit_points.size() == total_dofs,
	  ExcWrongFieldDimension (unit_points.size(), total_dofs));
  for (unsigned int d=0; d<dim; ++d)
    unit_points[0](d) = 0.5;
};
  

template <int dim>
void
FEDG_Q0<dim>::get_support_points (const typename DoFHandler<dim>::cell_iterator &cell,
				       vector<Point<dim> >  &support_points) const {
  Assert (support_points.size() == total_dofs,
	  ExcWrongFieldDimension (support_points.size(), total_dofs));
  
  support_points[0] = cell->center();
};


template <int dim>
void
FEDG_Q0<dim>::get_face_support_points (const typename DoFHandler<dim>::face_iterator &,
					    vector<Point<dim> >  &support_points) const {
  Assert ((support_points.size() == 0),
	  ExcWrongFieldDimension (support_points.size(),0));
};



template <int dim>
const FullMatrix<double> & 
FEDG_Q0<dim>::restrict (const unsigned int child) const {
  Assert (false, ExcNotImplemented());
  return restriction[child];
};



// explicit instantiations

template class FEDG_Q0<deal_II_dimension>;
