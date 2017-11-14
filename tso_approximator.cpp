#include "tso.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <cmath>
#include <set>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/bitset.hpp>

#include <flann/flann.hpp>

using namespace std;

typedef flann::Hamming<uint8_t> h_dist;
typedef h_dist::ElementType ElementType;
typedef h_dist::ResultType DistanceType;

const size_t test_case_number = TCASENMBR;
const size_t uint8s_per_tcase = UINT8TCASE;

vector <uint8_t> data_points;

set<size_t> unvisited_points;

vector<int> case_order;

const std::string SUCCESS_MSG = "Approximation completed successfully.";
const std::string ERROR_MSG = "Approximation failed. Exiting.";

ifstream fvector;

ofstream forder;

void export_case_order(){
  for(int& case_id : case_order){
    forder<<case_id;
    forder<<endl;
  }
  forder.close();
}

int main(int argc, char *argv[]){
  fvector.open(argv[1], ios::in | ios::binary);
  if(!fvector.is_open()){
    cerr<<"Error opening integer vector file."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }

  {
    cereal::BinaryInputArchive iarchive(fvector);
    iarchive(data_points);
  }
  
  flann::Matrix<ElementType> mtrx = flann::Matrix<ElementType>(&data_points[0], test_case_number, uint8s_per_tcase);
  flann::Index<h_dist> indx (mtrx, flann::LshIndexParams());
  indx.buildIndex();

  for (size_t i=0; i<test_case_number; i++){
    unvisited_points.insert(i);
  }

  size_t current_index = 0;
  ElementType* current_point;
  while (indx.size() > 1){
    current_point = indx.getPoint(current_index);
    indx.removePoint(current_index);
    case_order.push_back(current_index);
    unvisited_points.erase(current_index);
    flann::Matrix<ElementType> query = flann::Matrix<ElementType>(current_point, 1, uint8s_per_tcase);
    std::vector< std::vector<int> > indices;
    std::vector<std::vector<DistanceType> > dists;
    indx.knnSearch(query, indices, dists, 1, flann::SearchParams(flann::FLANN_CHECKS_UNLIMITED));
    if(indices[0].size() == 0){//Approximate neighbour was not found.
      current_index = *(unvisited_points.begin());
    }
    else{
      current_index = indices[0][0];
    }
  }
  case_order.push_back(current_index);

  forder.open(argv[2], ios::out);
  if(!forder.is_open()){
    cerr<<"Error opening approximated case order file."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }
  export_case_order();

  cout<<SUCCESS_MSG<<endl;
  return 0;
}
