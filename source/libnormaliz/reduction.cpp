/*
 * Normaliz
 * Copyright (C) 2007-2014  Winfried Bruns, Bogdan Ichim, Christof Soeger
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
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

//---------------------------------------------------------------------------

#include "reduction.h"

namespace libnormaliz {
using namespace std;

template<typename Integer>
Candidate<Integer>::Candidate(const vector<Integer>& v, const vector<Integer>& val, long sd){
    cand(v);
    values(val);
    sort_deg(sd);    
}

template<typename Integer>
Candidate<Integer>::Candidate(const vector<Integer>& v, const Full_Cone<Integer>& C){
    cand=v;
    values.resize(C.nrSupport_Hyperplanes);
    // cout << "size " << values.size();
    typename list<vector<Integer> >::const_iterator h;
    size_t i=0;
    for(h=C.Support_Hyperplanes.begin();h!=C.Support_Hyperplanes.end();++h){
    // cout << "*h " << *h;
    // cout << " v " << v;
        values[i]=v_scalar_product(v,*h);
        ++i;
    }
    sort_deg=explicit_cast_to_long<Integer>(v_scalar_product(v,C.Sorting));
}

/* template<typename Integer>
Candidate<Integer>::Candidate(const vector<Integer>& v, size_t nrSuppHyps){
    cand(v);
    values.resize(nrSuppHyps);   
} */

template<typename Integer>
CandidateList<Integer>::CandidateList(){
}

template<typename Integer>
CandidateList<Integer>::CandidateList(const list<vector<Integer> >& V_List, Full_Cone<Integer>& C){
                 
    typename list<vector<Integer> >::const_iterator v;
    
    for(v=V_List.begin();v!=V_List.end();++v)
        push_back(Candudate(*v,C));
    sort_it();
}


/*template<typename Integer>
void CandidateList<Integer>::insert(const vector<Integer>& v, Full_Cone<Integer>& C){
    insert(v,C.Hyperplanes,C.Sorting);
}

template<typename Integer>
void CandidateList<Integer>::insert(const vector<Integer>& v, const list<vector<Integer> >& SuppHyps, 
            const size_t& nrSuppHyps, const vector<Integer>& Sorting){

    typename list<vector<Integer> >::const_iterator h;
    Integer sd;
    vector<Integer> val(nrSuppHyps);
    size_t i=0;
    for(h=SuppHyps.begin();h!=SuppHyps.end();++h){
        val[i]=v_scalar_product(v,*h);
        ++i;
    }
    sd=explicit_cast_to_long<Integer>(v_scalar_product(*v,Sorting));
    Candidates.push_back(Candidate(v,val,sd));
} */

template<typename Integer>
bool CandidateList<Integer>::is_reducible(const vector<Integer>& v, const vector<Integer>& values, const long sort_deg) const {

    long sd=sort_deg/2;
    // cout << "-----------------" << endl << "Reduziere " << v << "sd " << sd << endl;
    typename list<Candidate<Integer> >::const_iterator r;
    for(r=Candidates.begin();r!=Candidates.end();++r){
        // cout << "gegen " << r->cand  << "sd " << r->sort_deg << endl;
        if(sd < r->sort_deg){
            // cout << "Ausgang 1 " << sort_deg << " " << sd << " " << r->sort_deg << endl;
            return(false);
        }
        size_t i=0;
        // cout << "cc " << values;
        // cout << "rr " << r->values;
        for(;i<values.size();++i)
            if(values[i]<r->values[i])
                break;
        if(i==values.size()){
            // cout << "Ausgang 2" << endl;
            return(true);
        }
   }
   // cout << "Ausgang 3" << endl;    
   return(false);    
}

template<typename Integer>
bool CandidateList<Integer>::is_reducible(const Candidate<Integer>& c) const {
    return((*this).is_reducible(c.cand, c.values, c.sort_deg));
}

template<typename Integer>
bool CandidateList<Integer>::is_reducible(vector<Integer> v,Candidate<Integer>& cand, const Full_Cone<Integer>& C) const {
    cand=Candidate<Integer>(v,C);
    return((*this).is_reducible(cand));
}

template<typename Integer>
void CandidateList<Integer>::reduce_by(CandidateList<Integer>& Reducers){

        // size_t ii=0;
        typename list<Candidate<Integer> >::iterator c;
        for(c=Candidates.begin();c!=Candidates.end();){
            // cout << ii << " " << c->cand; ++ii;
            if(Reducers.is_reducible(*c))
                c=Candidates.erase(c);
            else // continue
                ++c;
        }   
}

template<typename Integer>
void CandidateList<Integer>::auto_reduce(){
cout << "Size " << Candidates.size() << endl;
    reduce_by(*this);
}

template<typename Integer>
bool CandidateList<Integer>::reduce_by_and_insert(const Candidate<Integer>& cand, const CandidateList<Integer>& Reducers){
    bool irred=!Reducers.is_reducible(cand);
    if(irred)
        Candidates.push_back(cand);
    return irred;
}

template<typename Integer>
bool CandidateList<Integer>::reduce_by_and_insert(const vector<Integer>& v, Full_Cone<Integer>& C, CandidateList<Integer>& Reducers){
    Candidate<Integer> cand(v,C);
    return reduce_by_and_insert(cand,Reducers);
}

template<typename Integer>
bool cand_compare(const Candidate<Integer>& a, const Candidate<Integer>& b){
    return(a.sort_deg < b.sort_deg);
}

template<typename Integer>
void CandidateList<Integer>::sort_it(){
    Candidates.sort(cand_compare<Integer>);

}

template<typename Integer>
void CandidateList<Integer>::merge(CandidateList<Integer>& NewCand){
    Candidates.merge(NewCand.Candidates,cand_compare<Integer>);
}

template<typename Integer>
void CandidateList<Integer>::extract(list<vector<Integer> >& V_List){
    typename list<Candidate<Integer> >::iterator c;
    for(c=Candidates.begin();c!=Candidates.end();++c)
    V_List.push_back(c->cand);
                
}

template<typename Integer>
void CandidateList<Integer>::splice(CandidateList<Integer>& NewCand){
    Candidates.splice(Candidates.begin(),NewCand.Candidates);
}

 
} // namespace
