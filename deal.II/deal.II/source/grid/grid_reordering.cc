//----------------------------  grid_reordering.cc  ---------------------------
//    $Id$
//    Version: $Name$
//
//    Copyright (C) 2000 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//----------------------------  grid_reordering.cc  ---------------------------


#include <algorithm>
#include <stack>

#include <grid/grid_reordering.h>




template <int dim>
GridReordering<dim>::Cell::Cell () :
		cell_no (invalid_neighbor)
{
  for (unsigned int i=0; i<GeometryInfo<dim>::faces_per_cell; ++i)
    neighbors[i] = invalid_neighbor;
};



template <int dim>
GridReordering<dim>::Cell::Cell (const CellData<dim> &cd,
				 const unsigned int   cell_no) :
		CellData<dim> (cd), cell_no(cell_no)
{
  for (unsigned int i=0; i<GeometryInfo<dim>::faces_per_cell; ++i)
    neighbors[i] = invalid_neighbor;
};



template <int dim>
inline
unsigned int GridReordering<dim>::Cell::count_neighbors () const
{
  unsigned int n = 0;
  for (unsigned int i=0; i<GeometryInfo<dim>::faces_per_cell; ++i)
    if (neighbors[i] != invalid_neighbor)
      ++n;
  return n;
};



template <int dim>
void
GridReordering<dim>::Cell::insert_faces (map<Face,FaceData> &/*global_faces*/)
{
  Assert (false, ExcNotImplemented());
};



template <>
void
GridReordering<2>::Cell::insert_faces (map<Face,FaceData> &global_faces)
{
  const unsigned int dim = 2;

				   // first compute index numbers for
				   // the faces in usual order as
				   // defined by the order of vertices
				   // in the cell object
  Face new_faces[GeometryInfo<dim>::faces_per_cell]
    = { { {vertices[0], vertices[1]} },
        { {vertices[1], vertices[2]} },
	{ {vertices[3], vertices[2]} },
	{ {vertices[0], vertices[3]} } };

				   // then insert them into the global
				   // list and store iterators to
				   // them. note that if the face
				   // already exists, then the stored
				   // data is not touched.
  for (unsigned int face=0; face<GeometryInfo<dim>::faces_per_cell; ++face)
    faces[0][face] = global_faces.insert (make_pair(new_faces[face],
						    FaceData())).first;


				   // then for each of the faces also
				   // insert the reverse form and
				   // store pointers to them. note
				   // that the rotational state in
				   // which all faces are reverted is
				   // `2'
  for (unsigned int face=0; face<GeometryInfo<dim>::faces_per_cell; ++face)
    {
      swap (new_faces[face].vertices[0],
	    new_faces[face].vertices[1]);
      faces[2][face] = global_faces.insert (make_pair(new_faces[face],
						      FaceData())).first;
    };

				   // then finally fill in rotational
				   // states 1 and 3 of the cell. the
				   // faces of these states can be
				   // obtained from states 0 and 2
  faces[1][0] = faces[2][0];
  faces[1][1] = faces[0][1];
  faces[1][2] = faces[2][2];
  faces[1][3] = faces[0][3];
  
  faces[3][0] = faces[0][0];
  faces[3][1] = faces[2][1];
  faces[3][2] = faces[0][2];
  faces[3][3] = faces[2][3];
  

				   // finally fill the crosslink and
				   // other fields of the new
				   // entries. note that since
				   // rotational states 0 and 2 of the
				   // cell are exactly reverted, we
				   // only have to operate on the face
				   // pointers of these two states to
				   // reach all possible faces and
				   // permutations thereof
  for (unsigned int face=0; face<GeometryInfo<dim>::faces_per_cell; ++face)
    {
      if (faces[0][face]->second.adjacent_cells[0] ==
	  FaceData::invalid_adjacent_cell)
	{
					   // face had not been
					   // inserted by previous
					   // cells, since first
					   // adjacent cell is still
					   // untouched. provide
					   // xlinks to rotated faces
	  faces[0][face]->second.reverse_faces[0] = faces[2][face];
	  faces[2][face]->second.reverse_faces[0] = faces[0][face];

					   // and insert this cell as
					   // adjacent_cell of the faces
	  faces[0][face]->second.adjacent_cells[0] = cell_no;
	  faces[2][face]->second.adjacent_cells[0] = cell_no;
	}
      else
	{
					   // face had already been
					   // inserted. make sure that
					   // it was in the same way:
	  Assert (faces[0][face]->second.reverse_faces[0] == faces[2][face],
		  ExcInternalError());	  
	  Assert (faces[2][face]->second.reverse_faces[0] == faces[0][face],
		  ExcInternalError());

					   // now insert ourselves as
					   // second
					   // adjacent_cell. the
					   // respective slots must
					   // necessarily be empty
					   // still
	  Assert (faces[0][face]->second.adjacent_cells[1] ==
		  FaceData::invalid_adjacent_cell,
		  ExcInternalError());
	  Assert (faces[2][face]->second.adjacent_cells[1] ==
		  FaceData::invalid_adjacent_cell,
		  ExcInternalError());
	  faces[0][face]->second.adjacent_cells[1] = cell_no;
	  faces[2][face]->second.adjacent_cells[1] = cell_no;
	};
    };
};



template <int dim>
void GridReordering<dim>::Cell::fix_cell_neighbors ()
{
  for (unsigned int face=0; face<GeometryInfo<dim>::faces_per_cell; ++face)
    {
				       // first assert that the
				       // neighborship info of all
				       // versions of the same face is
				       // identical
      for (unsigned int rot=1; rot<rotational_states_of_faces; ++rot)
	for (unsigned int adjacent_cell=0; adjacent_cell<2; ++adjacent_cell)
	  Assert (faces[rot][face]->second.adjacent_cells[adjacent_cell]
		  ==
		  faces[0][face]->second.adjacent_cells[adjacent_cell],
		  ExcInternalError());
      

				       // then insert the neighbor
				       // behind this face as neighbor
				       // of the present cell. note
				       // that it is not relevant to
				       // which permutation of a face
				       // we refer. note that it might
				       // well be that some of the
				       // neighbor indices are
				       // FaceData::invalid_adjacent_cell
      if (faces[0][face]->second.adjacent_cells[0] == cell_no)
	neighbors[face] = faces[0][face]->second.adjacent_cells[1];
      else
	neighbors[face] = faces[0][face]->second.adjacent_cells[0];
    };
};



template <int dim>
void GridReordering<dim>::Cell::find_backtracking_point ()
{
				   // we know what neighbors we have,
				   // we can determine the neighbor
				   // with the maximal cell_no that is
				   // smaller than that of the present
				   // cell. we need this information
				   // in the backtracking process and
				   // don't want to compute it every
				   // time again
  track_back_to_cell = FaceData::invalid_adjacent_cell;
  for (unsigned int face=0; face<GeometryInfo<dim>::faces_per_cell; ++face)
    if ((neighbors[face] != FaceData::invalid_adjacent_cell)
	&&
	(neighbors[face] < cell_no)
	&&
	((neighbors[face] > track_back_to_cell)
	 ||
	 (track_back_to_cell == FaceData::invalid_adjacent_cell)))
      track_back_to_cell = neighbors[face];

				   // if this cell had no neighbors
				   // with lower cell numbers, we
				   // still need to know what cell to
				   // track back to in case some
				   // higher cell than the present one
				   // failed to coexist with the
				   // existing part of the mesh
				   // irrespective of the rotation
				   // state of this present cell. we
				   // then simply track back to the
				   // cell before this one, lacking a
				   // better alternative. this does,
				   // of course, not hold for cell 0,
				   // from which we should never be
				   // forced to track back
  if (track_back_to_cell == 0)
    track_back_to_cell = 0;
  else
    if (track_back_to_cell == FaceData::invalid_adjacent_cell)
      track_back_to_cell = cell_no-1;
};



template <int dim>
inline
bool GridReordering<dim>::Cell::check_consistency (const unsigned int /*rot*/) const
{
				   // might be that we can use the
				   // specialization for dim==2 after
				   // some modification, but haven't
				   // thought about that yet
  Assert (false, ExcNotImplemented());
  return false;
};



template <>
inline
bool GridReordering<2>::Cell::check_consistency (const unsigned int rot) const
{
  const unsigned int dim = 2;
				   // make sure that for each face of
				   // the cell the permuted faces are
				   // not already in use, as that
				   // would make the cell disallowed
  for (unsigned int face_no=0; face_no<GeometryInfo<dim>::faces_per_cell; ++face_no)
    {
      const FaceData &face = faces[rot][face_no]->second;

      for (unsigned int face_rot=0; face_rot<rotational_states_of_faces-1; ++face_rot)
	{
	  const FaceData &reverse_face = face.reverse_faces[face_rot]->second;
	  if (reverse_face.use_count != 0)
	    return false;
	};
    };

				   // no conflicts found
  return true;
};


template <int dim>
inline
void GridReordering<dim>::Cell::mark_faces_used (const unsigned int /*rot*/)
{
				   // might be that we can use the
				   // specialization for dim==2 after
				   // some modification, but haven't
				   // thought about that yet. in
				   // particular, I don't know whether
				   // we have to treat edges in 3d
				   // specially
  Assert (false, ExcNotImplemented());
};



template <int dim>
inline
void GridReordering<dim>::Cell::mark_faces_unused (const unsigned int /*rot*/)
{
				   // might be that we can use the
				   // specialization for dim==2 after
				   // some modification, but haven't
				   // thought about that yet. in
				   // particular, I don't know whether
				   // we have to treat edges in 3d
				   // specially
  Assert (false, ExcNotImplemented());
};



template <>
inline
void GridReordering<2>::Cell::mark_faces_used (const unsigned int rot)
{
  const unsigned int dim=2;
  for (unsigned int face=0; face<GeometryInfo<dim>::faces_per_cell; ++face)
    {
      Assert (faces[rot][face]->second.use_count < 2,
	      ExcInternalError());
      ++faces[rot][face]->second.use_count;
    };
};



template <>
inline
void GridReordering<2>::Cell::mark_faces_unused (const unsigned int rot)
{
  const unsigned int dim=2;
  for (unsigned int face=0; face<GeometryInfo<dim>::faces_per_cell; ++face)
    {
      Assert (faces[rot][face]->second.use_count > 0,
	      ExcInternalError());
      --faces[rot][face]->second.use_count;
    };
};



template <int dim>
bool GridReordering<dim>::Face::operator < (const Face &face) const
{
  for (unsigned int v=0; v<GeometryInfo<dim>::vertices_per_face; ++v)
    {
				       // if vertex index is smaller,
				       // then comparison is true
      if (vertices[v] < face.vertices[v])
	return true;
      else
					 // if vertex index is greater,
					 // then comparison is false
	if (vertices[v] > face.vertices[v])
	  return false;
				       // if indices are equal, then test
				       // next index
    };

				   // if all indices are equal:
  return false;
};



template <int dim>
GridReordering<dim>::FaceData::FaceData () :
		use_count (0)
{
  adjacent_cells[0] = adjacent_cells[1] = invalid_adjacent_cell;
};






template <int dim>
inline
void GridReordering<dim>::track_back (vector<Cell<dim> >                         &cells,
				      stack<unsigned int, vector<unsigned int> > &rotation_states,
				      unsigned int                                track_back_to_cell)
{
  top_of_function:
  
  Assert (track_back_to_cell > 0, ExcInternalError());

  unsigned int last_rotation_state;
  for (unsigned int cell_no=rotation_states.size()-1; cell_no>=track_back_to_cell; --cell_no)
    {
				       // store rotation state of
				       // topmost cell, as we will
				       // have to advance that by one
      last_rotation_state = rotation_states.top();
      
				       // first mark faces of that
				       // cell as no more used
      cells[cell_no].mark_faces_unused (last_rotation_state);

				       // then pop state from
				       // stack
      rotation_states.pop();
    };

				   // now we will have to find out
				   // whether we can try the last cell
				   // we have popped from the stack in
				   // another rotation state, or will
				   // have to backtrack further:
  if (last_rotation_state < rotational_states_of_cells-1)
    {
				       // possible. push that state to
				       // the stack and leave
      rotation_states.push (last_rotation_state+1);
      return;
    }
  else
    {
				       // last cell can't be rotated
				       // further. go on with
				       // backtracking
      const typename vector<Cell>::iterator
	try_cell = cells.begin() + rotation_states.size();
//        cout << "Further backtracking from " << rotation_states.size();
//        cout << ". Neighbors are ";
//        for (unsigned int i=0; i<GeometryInfo<dim>::faces_per_cell; ++i)
//  	cout << (int)try_cell->neighbors[i] << ' ';
//        cout << "Will track back to cell " << try_cell->track_back_to_cell << endl;

      track_back_to_cell = try_cell->track_back_to_cell;
      Assert (track_back_to_cell > 0, ExcInternalError());

				       // track further back. this
				       // could be done by recursive
				       // calls of this function,
				       // which in this case would
				       // represent a tail-recursion
				       // as there is nothing more to
				       // be done after calling the
				       // function recursively, but we
				       // prefer to write down the
				       // tail-recursion by hand using
				       // a goto, since the compiler
				       // seems to have problems to
				       // rewrite the tail recursion
				       // as a goto.
      goto top_of_function;
    };
};



template <int dim>
void GridReordering<dim>::find_reordering (vector<Cell<dim> >         &cells,
					   vector<CellData<dim> >     &original_cells,
					   const vector<unsigned int> &new_cell_numbers)
{
  const unsigned int n_cells = cells.size();
  
				   // stack of value indicating that
				   // the nth cell needs to be rotated
				   // so-and-so often, where n is the
				   // position on the stack
  stack<unsigned int, vector<unsigned int> > rotation_states;

				   // for the first cell, the
				   // rotational state can never be
				   // important, since we can rotate
				   // all other cells
				   // accordingly. therefore preset
				   // the rotation state of the first
				   // cell
  rotation_states.push (0);
  cells[0].mark_faces_used (rotation_states.top());
  
  while (true)
    {
				       // if all cells have a coherent
				       // orientation, then we can
				       // exit the main loop
      if (rotation_states.size() == n_cells)
	break;
      
				       // try to push back another
				       // cell in orientation zero
      rotation_states.push (0);

				       // check whether the present
				       // cell in the present
				       // orientation is valid
      check_topmost_cell:

      static unsigned int max_size = 0;
      if (rotation_states.size() > max_size)
	{
	  if (max_size % 10 == 0)
	    cout << "New max size " << rotation_states.size() << endl;
	  max_size = rotation_states.size();
	};
      
      const typename vector<Cell>::iterator
	try_cell = cells.begin() + rotation_states.size()-1;
      if (try_cell->check_consistency (rotation_states.top()))
	{
					   // yes, works, we found a
					   // way of how to add the
					   // present cell to the
					   // existing cells without
					   // violating any ordering
					   // constraints. now mark
					   // the respective faces as
					   // used and go on with the
					   // next cell
	  try_cell->mark_faces_used (rotation_states.top());
	  
//  	  cout << "Added cell " << try_cell->cell_no
//  	       << " in rotation " << rotation_states.top() << endl;
					   // go on with next cell
	  continue;
	}
      else
	{
					   // no, doesn't work. see if
					   // we can rotate the top
					   // cell so that it works
	  if (rotation_states.top()+1 < rotational_states_of_cells)
	    {
					       // yes, can be
					       // done. then do so and
					       // check again
	      ++rotation_states.top();
	      goto check_topmost_cell;
	    }
	  else
	    {
					       // no, no more
					       // orientation of the
					       // top cell possible,
					       // we have to backtrack
					       // some way
//  	      cout << "Failure with cell " << rotation_states.size()-1;
//  	      cout << ". Neighbors are ";
//  	      for (unsigned int i=0; i<GeometryInfo<dim>::faces_per_cell; ++i)
//  		cout << (int)try_cell->neighbors[i] << ' ';
//  	      cout << "Will track back to cell " << try_cell->track_back_to_cell << endl;

					       // first pop rotational
					       // state of top cell,
					       // since for that no
					       // faces have been
					       // marked as used yet
	      rotation_states.pop();
					       // then track back
	      track_back (cells, rotation_states, try_cell->track_back_to_cell);
					       // and go on by
					       // checking the now
					       // topmost cell
	      goto check_topmost_cell;
	    };
	};
    };



				   // rotate the cells according to
				   // the results we have found. since
				   // we operate on a stack, we do the
				   // rotations from the back of the
				   // array to the front
  while (rotation_states.size() != 0)
    {
      const unsigned int
	new_cell_number = rotation_states.size()-1;
      const unsigned int
	old_cell_number = find (new_cell_numbers.begin(),
				new_cell_numbers.end(),
				new_cell_number) - new_cell_numbers.begin();
      Assert (old_cell_number < cells.size(), ExcInternalError());

      original_cells[old_cell_number].rotate (rotation_states.top());
      rotation_states.pop ();
    };
};



template <int dim>
vector<unsigned int>
GridReordering<dim>::presort_cells (vector<Cell<dim> > &cells,
				    map<Face,FaceData> &faces)
{
				   // first find the cell with the
				   // least neighbors
  unsigned int min_neighbors           = cells[0].count_neighbors();
  unsigned int cell_with_min_neighbors = 0;
  for (unsigned int i=1; i<cells.size(); ++i)
    if (min_neighbors > cells[i].count_neighbors())
      {  
	min_neighbors = cells[i].count_neighbors();
	cell_with_min_neighbors = i;
	if (min_neighbors == 1)
					   // better is not possible
	  break;
      };

				   // have an array into which we
				   // insert the new cells numbers of
				   // each cell
  const unsigned int invalid_cell_number = static_cast<unsigned int>(-1);
  vector<unsigned int> new_cell_numbers (cells.size(), invalid_cell_number);

				   // and have an array of the next
				   // cells to be numbered (old numbers)
  vector<unsigned int> next_round_cells (1, cell_with_min_neighbors);

  unsigned int next_free_new_number = 0;
  
				   // while there are still cells to
				   // be renumbered:
  while (next_round_cells.size() != 0)
    {
      for (unsigned int i=0; i<next_round_cells.size(); ++i)
	{
	  Assert (new_cell_numbers[next_round_cells[i]] == invalid_cell_number,
		  ExcInternalError());
	  
	  new_cell_numbers[next_round_cells[i]] = next_free_new_number;
	  ++next_free_new_number;
	};

				       // for the next round, find all
				       // neighbors of the cells of
				       // this round which have not
				       // yet been renumbered
      vector<unsigned int> new_next_round_cells;
      for (unsigned int i=0; i<next_round_cells.size(); ++i)
	for (unsigned int n=0; n<GeometryInfo<dim>::faces_per_cell; ++n)
	  if (cells[next_round_cells[i]].neighbors[n] != Cell<dim>::invalid_neighbor)
	    if (new_cell_numbers[cells[next_round_cells[i]].neighbors[n]]
		== invalid_cell_number)
	      new_next_round_cells.push_back (cells[next_round_cells[i]].neighbors[n]);

	    
      
				       // if no more cells have been
				       // found, then we must have
				       // renumbered all cells already
      if (new_next_round_cells.size() == 0)
	Assert (next_free_new_number == cells.size(), ExcInternalError());

				       // eliminate duplicates from
				       // the new_next_round_cells
				       // array. note that a cell
				       // which is entered into this
				       // array might have been
				       // entered more than once since
				       // it might be a neighbor of
				       // more than one cell of the
				       // present round
				       //
				       // in order to eliminate
				       // duplicates, we first sort
				       // tha array and then copy over
				       // only unique elements to the
				       // next_round_cells array,
				       // which is needed for the next
				       // loop iteration anyway
      sort (new_next_round_cells.begin(), new_next_round_cells.end());
      next_round_cells.clear ();
      unique_copy (new_next_round_cells.begin(), new_next_round_cells.end(),
		   back_inserter(next_round_cells));
    };
  Assert (find (new_cell_numbers.begin(), new_cell_numbers.end(), invalid_cell_number)
	  ==
	  new_cell_numbers.end(),
	  ExcInternalError());

				   // now that we know in which order
				   // to sort the cells, do so:
  vector<Cell<dim> > new_cells (cells.size());
  for (unsigned int i=0; i<cells.size(); ++i)
    new_cells[new_cell_numbers[i]] = cells[i];
				   // then switch old and new array
  swap (cells, new_cells);
  
				   // now we still have to convert all
				   // old cell numbers to new cells
				   // numbers
  for (unsigned int c=0; c<cells.size(); ++c)
    {
      cells[c].cell_no = new_cell_numbers[cells[c].cell_no];
      Assert (cells[c].cell_no == c, ExcInternalError());

      for (unsigned int n=0; n<GeometryInfo<dim>::faces_per_cell; ++n)
	cells[c].neighbors[n] = new_cell_numbers[cells[c].neighbors[n]];
    };

  for (typename map<Face,FaceData>::iterator i=faces.begin(); i!=faces.end(); ++i)
    for (unsigned int k=0; k<2; ++k)
      if (i->second.adjacent_cells[k] != FaceData::invalid_adjacent_cell)
	i->second.adjacent_cells[k] = new_cell_numbers[i->second.adjacent_cells[k]];

  return new_cell_numbers;
};

		      

template <int dim>
void GridReordering<dim>::reorder_cells (vector<CellData<dim> > &original_cells)
{
				   // the present function might
				   // actually work in 3d, but the
				   // ones it calls probably not. will
				   // have to think about what needs
				   // to be changed. however, rather
				   // than killing the program when
				   // calling functions that may not
				   // work, kill it here if the
				   // dimension is not appropriate
  Assert (dim==2, ExcNotImplemented());
  
				   // we need more information than
				   // provided by the input parameter,
				   // in particular we need
				   // neighborship relations between
				   // cells. therefore copy over the
				   // old cells to another class that
				   // provides space to these
				   // informations
  vector<Cell<dim> > cells;
  cells.reserve (original_cells.size());
  for (unsigned int i=0; i<original_cells.size(); ++i)
    cells.push_back (Cell<dim>(original_cells[i], i));
  
				   // first generate all the faces
				   // possible, i.e. in each possible
				   // direction and rotational state
  map<Face,FaceData> faces;
  for (unsigned int cell_no=0; cell_no<cells.size(); ++cell_no)
    cells[cell_no].insert_faces (faces);

				   // after all faces have been filled
				   // and the faces have indices of
				   // their neighbors, we may also
				   // insert the neighbor indices into
				   // the cells themselves
  for (unsigned int cell_no=0; cell_no<cells.size(); ++cell_no)
    cells[cell_no].fix_cell_neighbors ();


				   // do a preordering step in order
				   // to make further backtracking
				   // more local
  const vector<unsigned int>
    new_cell_numbers = presort_cells (cells, faces);

				   // finally do some preliminary work
				   // to make backtracking simpler
				   // later
  for (unsigned int cell_no=0; cell_no<cells.size(); ++cell_no)
    cells[cell_no].find_backtracking_point ();
  
				   // now do the main work
  find_reordering (cells, original_cells, new_cell_numbers);
};



template <>
void GridReordering<1>::reorder_cells (vector<CellData<1> > &)
{
				   // there should not be much to do
				   // in 1d...
};




// explicit instantiations. only require the main function, it should
// then claim whatever templates it needs
template
void
GridReordering<deal_II_dimension>::
reorder_cells (vector<CellData<deal_II_dimension> > &);

