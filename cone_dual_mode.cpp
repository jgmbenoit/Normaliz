/*
 * Normaliz 2.2
 * Copyright (C) 2007,2008,2009  Winfried Bruns, Bogdan Ichim
 * With contributions by Christof Soeger
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//---------------------------------------------------------------------------

#include <stdlib.h>
#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

//---------------------------------------------------------------------------

#include "cone_dual_mode.h"
#include "vector_operations.h"
#include "lineare_transformation.h"
#include "list_operations.h"

//---------------------------------------------------------------------------
extern bool test_arithmetic_overflow;
extern int overflow_test_modulus;
extern int lifting_bound;
extern bool verbose;
extern bool optimize_speed;
extern void global_error_handling();

//---------------------------------------------------------------------------
//private
//---------------------------------------------------------------------------

bool Cone_Dual_Mode::reduce( list< vector< Integer >* >& Ired, const vector< Integer >& new_element, const int& size){
	register int i,c=1;
	list< vector<Integer>* >::iterator j;
	vector<Integer> *reducer;
	for (j =Ired.begin(); j != Ired.end(); j++) {
		reducer=(*j);
		if (new_element[0]<=(*reducer)[0])
			continue;
		if ((*reducer)[c]<=new_element[c]){
			for (i = 1; i <=size ; i++) {
				if ((*reducer)[i]>new_element[i]){
					c=i;
					break;
				}
			}
			if (i==size+1) {
				Ired.push_front(*j);
				Ired.erase(j);
				return true;
			}
		}
		//new_element is reducible
	}
	return false;
}

//---------------------------------------------------------------------------

bool Cone_Dual_Mode::reduce( list< vector< Integer > >& Ired, const vector< Integer >& new_element, const int& size){
	register int i,c=1;
	list< vector<Integer> >::iterator j;
	for (j =Ired.begin(); j != Ired.end(); j++) {
		if (new_element[0]<=(*j)[0])
			continue;
		if ((*j)[c]<=new_element[c]){
			for (i = 1; i <=size ; i++) {
				if ((*j)[i]>new_element[i]){
					c=i;
					break;
				}
			}
			if (i==size+1) {
				Ired.push_front(*j);
				Ired.erase(j);
				return true;
			}
		}
		//new_element is reducible
	}
	return false;
}

//---------------------------------------------------------------------------

void Cone_Dual_Mode::reduce( list< vector< Integer > >& Ired, list< vector< Integer > >& Red, const int& size){
	Ired.sort();
	register int i,c=1;
	vector<Integer> dummy(size+3,0);
	Red.push_front(dummy);
	Red.push_back(dummy);
	list< vector<Integer> >::iterator j;
	list< vector<Integer> >::iterator s;
	for (s = Red.begin(); s != Red.end(); s++) {
		for (j =Ired.begin(); j != Ired.end(); j++) {
			if ((*s)[0]<2*(*j)[0]) {
				break; //element is not reducible;
			}
			else  {

				if ((*j)[c]<=(*s)[c]){
					for (i = 1; i <= size; i++) {
						if ((*j)[i]>(*s)[i]){
							c=i;
							break;
						}
					}
					if (i==size+1) {
						Ired.push_front(*j);
						Ired.erase(j);
						s=Red.erase(s);
						//	if(s!=Red.begin())
						s--;
						break;
					}
				}
				//new_element is not in the Hilbert Basis
			}
		}
	}
	Red.pop_front();
	Red.pop_back();
}

//---------------------------------------------------------------------------

void Cone_Dual_Mode::reduce_and_insert(const vector< Integer >& new_element, const int& size){
	register int i,c=1;
	list< vector<Integer> >::iterator j;
	for (j =Hilbert_Basis.begin(); j != Hilbert_Basis.end(); j++) {
		if (new_element[0]<2*(*j)[0]) {
			break; //new_element is not reducible;
		}
		else  {

			if ((*j)[c]<=new_element[c]){
				for (i = 1; i <= size; i++) {
					if ((*j)[i]>new_element[i]){
						c=i;
						break;
					}
				}
				if (i==size+1) {
					Hilbert_Basis.push_front(*j);
					Hilbert_Basis.erase(j);
					return;
				}
			}
			//new_element is not in the Hilbert Basis
		}
	}
	Hilbert_Basis.push_back(new_element);
}

//---------------------------------------------------------------------------

void Cone_Dual_Mode::reduce_and_insert_extreme( const vector< Integer >& new_element){
	register int i,c=1;
	list< vector<Integer> >::iterator j;
	for (j =Support_Hyperplanes.begin(); j != Support_Hyperplanes.end(); j++) {
		if (new_element[0]<=(*j)[0])
			continue;
		if ((*j)[c]<=new_element[c]){
			for (i = 1; i <=nr_gen ; i++) {
				if ((*j)[i]>new_element[i]){
					c=i;
					break;
				}
			}
			if (i==nr_gen+1) {
				Support_Hyperplanes.push_front(*j);
				Support_Hyperplanes.erase(j);
				return; //new element is not an extreme ray
			}
		}
		//new_element is reducible
	}
	Support_Hyperplanes.push_back(new_element);
}

//---------------------------------------------------------------------------
//public
//---------------------------------------------------------------------------

Cone_Dual_Mode::Cone_Dual_Mode(){
	dim=0;
	nr_gen=0;
	hyp_size=0;
}

//---------------------------------------------------------------------------

Cone_Dual_Mode::Cone_Dual_Mode(Matrix M){
	dim=M.nr_of_columns();
	if (dim!=M.rank()) {
		error("error: Matrix with rank = number of columns needed in the constructor of the object Cone_Dual_Mode.");
	}
	Generators = M;
	nr_gen=Generators.nr_of_rows();
	//make the generators coprime and remove 0 rows
	vector<Integer> gcds = Generators.make_prime();
	vector<int> key=v_non_zero_pos(gcds);
	if (key.size() < nr_gen) {
		Generators=Generators.submatrix(key);
		nr_gen=Generators.nr_of_rows();
	}
	hyp_size=dim+nr_gen;
	Support_Hyperplanes = list< vector<Integer> >();
	Hilbert_Basis = list< vector<Integer> >();
}

//---------------------------------------------------------------------------

Cone_Dual_Mode::Cone_Dual_Mode(const Cone_Dual_Mode& C){
	dim=C.dim;
	nr_gen=C.nr_gen;
	hyp_size=C.hyp_size;
	Generators=C.Generators;
	Support_Hyperplanes=C.Support_Hyperplanes;
	Hilbert_Basis=C.Hilbert_Basis;
}

//---------------------------------------------------------------------------

Cone_Dual_Mode::~Cone_Dual_Mode(){
	//automatic destructor
}

//---------------------------------------------------------------------------

void Cone_Dual_Mode::print()const{
	cout<<"dim="<<dim<<".\n";
	cout<<"nr_gen="<<nr_gen<<".\n";
	cout<<"hyp_size="<<hyp_size<<".\n";
	cout<<"Generators are:\n";
	Generators.read();cout<<endl;
	cout<<"Support Hyperplanes are:\n";
	l_read(Support_Hyperplanes);cout<<endl;
	cout<<"Hilbert Basis is:\n";
	l_read(Hilbert_Basis);cout<<endl;
}

//---------------------------------------------------------------------------

int Cone_Dual_Mode::read_dimension()const{
	return dim;
}

//---------------------------------------------------------------------------

int Cone_Dual_Mode::read_nr_generators()const{
	return nr_gen;
}

//---------------------------------------------------------------------------

Matrix Cone_Dual_Mode::read_generators()const{
	return Generators;
}

//---------------------------------------------------------------------------

Matrix Cone_Dual_Mode::read_support_hyperplanes()const{
	int s= Support_Hyperplanes.size();
	Matrix M(s,dim);
	int i=1;
	list< vector<Integer> >::const_iterator l;
	for (l =Support_Hyperplanes.begin(); l != Support_Hyperplanes.end(); l++) {
		M.write(i,(*l));
		i++;
	}
	return M;
}

//---------------------------------------------------------------------------

Matrix Cone_Dual_Mode::read_hilbert_basis()const{
	int s= Hilbert_Basis.size();
	Matrix M(s,dim);
	int i=1;
	list< vector<Integer> >::const_iterator l;
	for (l =Hilbert_Basis.begin(); l != Hilbert_Basis.end(); l++) {
		M.write(i,(*l));
		i++;
	}
	return M;
}

//---------------------------------------------------------------------------

void Cone_Dual_Mode::cut_with_halfspace_hilbert_basis(const int& hyp_counter, const bool& lifting, vector<Integer>& halfspace){
	if (verbose==true) {
		cout<<"cut with halfspace "<<hyp_counter<<" ..."<<endl;
	}
	int i,sign;
	bool  not_done;
	list < vector <Integer> > Positive_Ired,Negative_Ired,Neutral_Ired;
	Integer orientation, scalar_product,diff,factor;
	vector <Integer> hyperplane=Generators.read(hyp_counter);
	list< vector<Integer> >::iterator h;
	if (lifting==true) {
		orientation=v_scalar_product(hyperplane,halfspace);
		if(orientation<0){
			orientation=-orientation;
			v_scalar_multiplication(halfspace,-1); //transforming into the generator of the positive halfspace
		}
		for (h = Hilbert_Basis.begin(); h != Hilbert_Basis.end(); ++h) { //reduction  modulo  the generator of the positive halfspace
			scalar_product=v_scalar_product_unequal_vectors_end(hyperplane,(*h));
			sign=1;
			if (scalar_product<0) {
				scalar_product=-scalar_product;
				sign=-1;
			}
			factor=scalar_product/orientation;
			for (i = 0; i < dim; i++) {
				(*h)[nr_gen+3+i]=(*h)[nr_gen+3+i]-sign*factor*halfspace[i];
			}
		}
		//adding the generators of the halfspace to negative and positive
		vector <Integer> hyp_element(hyp_size+3,0);
		for (i = 0; i < dim; i++) {
			hyp_element[nr_gen+3+i]= halfspace[i];
		}
		hyp_element[hyp_counter]=orientation;
		hyp_element[0]=orientation;
		if (orientation==0){ //never
			Neutral_Ired.push_back(hyp_element);
		}
		else{
			Positive_Ired.push_back(hyp_element);
			v_scalar_multiplication(hyp_element,-1);
			hyp_element[hyp_counter]=orientation;
			hyp_element[0]=orientation;
			Negative_Ired.push_back(hyp_element);
		}
	}
	for (h = Hilbert_Basis.begin(); h != Hilbert_Basis.end(); ++h) { //dividing into negative and positive
		(*h)[hyp_counter]=v_scalar_product_unequal_vectors_end(hyperplane,(*h));
		if ((*h)[hyp_counter]>0) {
			(*h)[nr_gen+1]=1;     // generation
			(*h)[nr_gen+2]=0;     //not sum
			(*h)[0]+=(*h)[hyp_counter];
			Positive_Ired.push_back((*h));
		}
		if ((*h)[hyp_counter]<0) {
			(*h)[hyp_counter]=-(*h)[hyp_counter];
			(*h)[nr_gen+1]=1;
			(*h)[nr_gen+2]=0;
			(*h)[0]+=(*h)[hyp_counter];
			Negative_Ired.push_back((*h));
		}
		if ((*h)[hyp_counter]==0) {
			(*h)[nr_gen+1]=0;
			(*h)[nr_gen+2]=0;
			Neutral_Ired.push_back((*h));
		}
	}
	Neutral_Ired.sort();
	Positive_Ired.sort();
	Negative_Ired.sort();
	//long int counter=0;
	list < vector <Integer> > New_Positive,New_Negative,New_Neutral,Pos,Neg,Neu;
	list < vector<Integer> >::const_iterator p,n;
	list < vector <Integer> >::iterator c;
	not_done=true;
	while(not_done){
		not_done=false;
		New_Positive.clear();
		New_Negative.clear();
		New_Neutral.clear();
		//generating new elements
//		cout<<"+"<<flush;
//		for(p = Positive_Ired.begin(); p != Positive_Ired.end(); p++){
		list < vector<Integer>* > Positive,Negative,Neutral; // pointer lists, used to move reducers to the front
		list < vector<Integer> >::iterator it;
		it=Positive_Ired.begin();
		while (it!=Positive_Ired.end()) {
			Positive.push_back(&(*(it++)));
		}
		it=Negative_Ired.begin();
		while (it!=Negative_Ired.end()) {
			Negative.push_back(&(*(it++)));
		}
		it=Neutral_Ired.begin();
		while (it!=Neutral_Ired.end()) {
			Neutral.push_back(&(*(it++)));
		}
		int psize=Positive_Ired.size();
		#pragma omp parallel private(p,n,diff) firstprivate(Positive,Negative,Neutral)
		{
		int ppos=0;
		p = Positive_Ired.begin();
		#pragma omp for schedule(dynamic)
		for(i = 0; i<psize; ++i){
			for(;i > ppos; ++ppos, ++p) ;
			for(;i < ppos; --ppos, --p) ;

			for (n = Negative_Ired.begin(); n != Negative_Ired.end(); ++n){
				if ((*p)[nr_gen+1]<=1&&(*n)[nr_gen+1]<=1&&((*p)[nr_gen+1]!=0||(*n)[nr_gen+1]!=0)) {
					if (((*p)[nr_gen+2]!=0&&(*p)[nr_gen+2]<=(*n)[hyp_counter])||((*n)[nr_gen+2]!=0&&(*n)[nr_gen+2]<=(*p)[hyp_counter]))
						continue;
					//	counter++;
					diff=(*p)[hyp_counter]-(*n)[hyp_counter];
					vector <Integer> new_candidate=v_add((*p),(*n));

					if (diff>0) {
						new_candidate[hyp_counter]=diff;
						new_candidate[0]-=2*(*n)[hyp_counter];
						if(reduce(Positive,new_candidate,hyp_counter)) {
							continue;
						}
						if(reduce(Neutral,new_candidate,hyp_counter-1)) {
							continue;
						}
						new_candidate[nr_gen+1]=2;
						new_candidate[nr_gen+2]=(*p)[hyp_counter];
						#pragma omp critical(NEW_POSITIVE)
						New_Positive.push_back(new_candidate);
					}
					if (diff<0) {
						new_candidate[hyp_counter]=-diff;
						new_candidate[0]-=2*(*p)[hyp_counter];
						if(reduce(Negative,new_candidate,hyp_counter)) {
							continue;
						}
						if(reduce(Neutral,new_candidate,hyp_counter-1)) {
							continue;
						}
						new_candidate[nr_gen+1]=2;
						new_candidate[nr_gen+2]=(*n)[hyp_counter];
						#pragma omp critical(NEW_NEGATIVE)
						New_Negative.push_back(new_candidate);
					}
					if (diff==0) {
						new_candidate[hyp_counter]=0;
						new_candidate[0]-=2*(*p)[hyp_counter];
						if(reduce(Neutral,new_candidate,hyp_counter-1)) {
							continue;
						}
						new_candidate[nr_gen+1]=0;
						new_candidate[nr_gen+2]=0;
						#pragma omp critical(NEW_NEUTRAL)
						New_Neutral.push_back(new_candidate);
					}
				}
			}
		}
		//end generation of new elements
		#pragma omp single nowait
		New_Neutral.sort();
		#pragma omp single nowait
		New_Positive.sort();
		#pragma omp single nowait
		New_Negative.sort();
		} //END PARALLEL
//		cout<<"-"<<flush;
		//reducing the new vectors agains them self
		//Neutral_Ired=Neutral;
		//Positive_Ired=Positive;
		//Negative_Ired=Negative;
		/*reduce(Neutral,New_Neutral,hyp_counter-1);
		  reduce(Neutral,New_Positive,hyp_counter-1);
		  reduce(Neutral,New_Negative,hyp_counter-1);
		  New_Positive.sort();
		  New_Negative.sort();
		  reduce(Positive,New_Positive,hyp_counter);
		  reduce(Negative,New_Negative,hyp_counter);
		  New_Neutral.sort();
		  New_Positive.sort();
		  New_Negative.sort();
		  New_Neutral.merge(Neu);
		  New_Positive.merge(Pos);
		  New_Negative.merge(Neg);*/
		Hilbert_Basis.clear();
		for (c=New_Neutral.begin(); c != New_Neutral.end(); ++c) {
			reduce_and_insert((*c),hyp_counter-1);
		}
		New_Neutral=Hilbert_Basis;
		Hilbert_Basis.clear();
		for (c=New_Positive.begin(); c != New_Positive.end(); ++c) {
			reduce_and_insert((*c),hyp_counter);
		}
		New_Positive=Hilbert_Basis;
		Hilbert_Basis.clear();
		for (c=New_Negative.begin(); c != New_Negative.end(); ++c) {
			reduce_and_insert((*c),hyp_counter);
		}
		New_Negative=Hilbert_Basis;
		if (New_Neutral.size()!=0) {
			New_Positive.sort();
			reduce(New_Neutral,New_Positive, hyp_counter-1);
			New_Negative.sort();
			reduce(New_Neutral,New_Negative, hyp_counter-1);
			reduce(New_Neutral,Neutral_Ired, hyp_counter-1);
			reduce(New_Neutral,Positive_Ired, hyp_counter-1);
			reduce(New_Neutral,Negative_Ired, hyp_counter-1);
			Neutral_Ired.merge(New_Neutral);
		}
		if (New_Positive.size()!=0) {
			not_done=true;
			reduce(New_Positive,Positive_Ired, hyp_counter);
			Positive_Ired.merge(New_Positive);
		}
		if (New_Negative.size()!=0) {
			not_done=true;
			reduce(New_Negative,Negative_Ired, hyp_counter);
			Negative_Ired.merge(New_Negative);
		}
		for (c = Positive_Ired.begin(); c != Positive_Ired.end(); ++c){
			if((*c)[nr_gen+1]>0) {
				(*c)[nr_gen+1]--;
			}
		}
		for (c = Negative_Ired.begin(); c != Negative_Ired.end(); ++c){
			if((*c)[nr_gen+1]>0) {
				(*c)[nr_gen+1]--;
			}
		}
//		cout<<not_done;
	}
	//still possible to have double elements in the Hilbert basis, coming from different generations

	set< vector<Integer> > Help;
	set< vector<Integer> >::iterator d;
	for (c = Positive_Ired.begin(); c != Positive_Ired.end(); ++c) {
		(*c)[nr_gen+1]=0;
		(*c)[nr_gen+2]=0;
		Help.insert(*c);
	}
	for (c = Neutral_Ired.begin(); c != Neutral_Ired.end(); ++c) {
		(*c)[nr_gen+1]=0;
		(*c)[nr_gen+2]=0;
		Help.insert(*c);
	}
	Hilbert_Basis.clear();
	d=Help.begin();
	while(d != Help.end()) {
		Hilbert_Basis.push_back(*d);
		Help.erase(d);
		d=Help.begin();
	}
	if (verbose==true) {
		cout<<"Hilbert basis size="<<Hilbert_Basis.size()<<endl;
	}
}

//---------------------------------------------------------------------------

Matrix Cone_Dual_Mode::cut_with_halfspace(const int& hyp_counter, const Matrix& Basis_Max_Subspace){
	int i,j,rank_subspace=Basis_Max_Subspace.nr_of_rows();
	vector <Integer> scalar_product,hyperplane=Generators.read(hyp_counter),halfspace;
	bool lifting=false;
	Matrix New_Basis_Max_Subspace=Basis_Max_Subspace;
	if (rank_subspace!=0) {
		scalar_product=Basis_Max_Subspace.MxV(hyperplane);
		for (i = 0; i <rank_subspace; i++)
			if (scalar_product[i]!=0)
				break;
		if (i!=rank_subspace) {    // the new hyperplane is not contained in the maximal subspace
			lifting=true;
			//computing new maximal subspace
			Matrix M(1,rank_subspace);
			M.write(1,scalar_product);
			Lineare_Transformation LT=Transformation(M);
			Matrix Lifted_Basis_Factor_Space_over_Ker_and_Ker=LT.get_right();
			Lifted_Basis_Factor_Space_over_Ker_and_Ker=Lifted_Basis_Factor_Space_over_Ker_and_Ker.transpose();
			Matrix  Ker(rank_subspace-1,rank_subspace);
			for (j = 1; j <= rank_subspace-1; j++) {
				Ker.write(j, Lifted_Basis_Factor_Space_over_Ker_and_Ker.read(j+1));
			}
			New_Basis_Max_Subspace=Ker.multiplication(Basis_Max_Subspace);
			halfspace=Basis_Max_Subspace.VxM(Lifted_Basis_Factor_Space_over_Ker_and_Ker.read(1));
		}
	}
	cut_with_halfspace_hilbert_basis(hyp_counter, lifting, halfspace);
	return New_Basis_Max_Subspace;
}

//---------------------------------------------------------------------------

void Cone_Dual_Mode::extreme_rays_reduction(){
	list < vector <Integer> >::iterator c;
	int i,k;
	for (c=Hilbert_Basis.begin(); c!=Hilbert_Basis.end(); ++c){
		k=0;
		for (i = 1; i <= nr_gen; i++) {
			if ((*c)[i]!=0) {
				(*c)[i]=1;
				k++;
			}

		}
		(*c)[0]=k;       //if (k>=dim-1) improuves speed much here
	}
	Hilbert_Basis.sort();
	for (c=Hilbert_Basis.begin(); c!=Hilbert_Basis.end(); ++c){
		reduce_and_insert_extreme((*c));
	}
	l_cut_front(Support_Hyperplanes,dim);
}

//---------------------------------------------------------------------------

void Cone_Dual_Mode::extreme_rays_rank(){
	list < vector <Integer> >::iterator c;
	list <int> zero_list;
	int i,j,k;
	for (c=Hilbert_Basis.begin(); c!=Hilbert_Basis.end(); ++c){
		zero_list.clear();
		for (i = 1; i <= nr_gen; i++) {
			if ((*c)[i]==0) {
				zero_list.push_back(i);
			}
		}
		k=zero_list.size();
		if (k>=dim-1) {
			vector <int> zero_vector(k);
			for (j = 0; j < k; j++) {
				zero_vector[j]=zero_list.front();
				zero_list.pop_front();
			}
			Matrix Test=Generators.submatrix(zero_vector);
			if (Test.rank()>=dim-1) {
				Support_Hyperplanes.push_back((*c));
			}
		}
	}
	l_cut_front(Support_Hyperplanes,dim);
}

//---------------------------------------------------------------------------

void Cone_Dual_Mode::hilbert_basis_dual(){
if(dim>0){            //correction needed to include the 0 cone;
	if (verbose==true) {
		cout<<"\n************************************************************\n";
		cout<<"computing Hilbert basis ..."<<endl;
	}
	int hyp_counter;      // current hyperplane
	Matrix Basis_Max_Subspace(dim);      //identity matrix
	for (hyp_counter = 1; hyp_counter <= nr_gen; hyp_counter++) {
		Basis_Max_Subspace=cut_with_halfspace(hyp_counter,Basis_Max_Subspace);
	}
	extreme_rays_rank();
	l_cut_front(Hilbert_Basis,dim);
}
}



//---------------------------------------------------------------------------

void Cone_Dual_Mode::error(string s) const{
	cerr <<"\nCone_Dual_Mode: "<< s<<"\n";
	global_error_handling();
}

