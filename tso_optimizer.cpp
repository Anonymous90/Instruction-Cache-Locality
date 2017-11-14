#include "tso.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cmath>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/bitset.hpp>

using namespace std;

//TODO include all signatures in header file.
float get_case_distance(int, int);
int get_case_rank(int);

struct distance_operator{
  distance_operator(int base_case){
    this->base_case = base_case;
  }
  bool operator()(int lhs, int rhs)const{
    if(lhs == rhs){
      return false;
    }
    if(get_case_distance(base_case, lhs) == get_case_distance(base_case, rhs)){
      return lhs < rhs;
    }
    return get_case_distance(base_case, lhs) < get_case_distance(base_case, rhs);
  }
  int base_case;
};

struct rank_operator{
  bool operator ()(int lhs, int rhs)const{ 
    if(lhs == rhs){
      return false;
    }
    if(get_case_rank(lhs) == get_case_rank(rhs)){
      return lhs > rhs;
    }
    return get_case_rank(lhs) > get_case_rank(rhs);
  }
};

const float average_instructions_executed = AVGINS;
const size_t bbl_universe_size = BBLUNIV;
vector< bitset<bbl_universe_size> > bbl_vector;

vector<int> case_order;
vector<vector<float> > case_distances;
vector<int> case_ranks;
vector<set<int, distance_operator> > case_similarities;
set<int, rank_operator> ranked_cases;

const std::string SUCCESS_MSG = "Optimization completed successfully.";
const std::string ERROR_MSG = "Optimization failed. Exiting.";

int test_number=0;
int average_ins_size=0;
int l1_ins_cache_size=0;
int distances_number=0;
int distances_below_threshold=0;
float distances_below_threshold_percentage=0.0;
float threshold=0.0;
float cache_instruction_capacity=0.0;
float distances_avg=0.0;
float low_threshold=0.20;

ifstream fvector;
ofstream forder, foptdetails;

void export_optimization_details(){
  if(distances_number > 0){
    distances_avg = distances_avg / distances_number;
    distances_below_threshold_percentage = (( (float) distances_below_threshold / (float) distances_number) * 100.0);
  }
  foptdetails<<"Calculated threshold: "<<threshold<<endl<<endl;
  foptdetails<<"Average instruction size(in bytes): "<<average_ins_size<<endl<<endl;
  foptdetails<<"L1 instruction cache size(in bytes): "<<l1_ins_cache_size<<endl<<endl;
  foptdetails<<"Cache instruction capacity(instruction number): "<<cache_instruction_capacity<<endl<<endl;
  foptdetails<<"Test case number: "<<test_number<<endl<<endl;
  foptdetails<<"Case distances number: "<<distances_number<<endl<<endl;
  foptdetails<<"Case distances below threshold: "<<distances_below_threshold<<" ("<<distances_below_threshold_percentage<<"%)"<<endl<<endl;
  foptdetails<<"Case distance average: "<<distances_avg<<endl<<endl;
  foptdetails.close();
}

void export_case_order(){
  for(int& case_id : case_order){
    forder<<case_id;
    forder<<endl;
  }
  forder.close();
}

float get_case_distance(int case1, int case2){
  if(case1 == case2){
    return 0.0;
  }
  if(case1 > case2){
    return case_distances.at(case2).at(case1-case2-1);
  }
  return case_distances.at(case1).at(case2-case1-1);
}

int get_case_rank(int case1){
  return case_ranks[case1];
}

void visit_test_case(int test_case){
  case_order.push_back(test_case);
  ranked_cases.erase(test_case);
  if(case_ranks[test_case] > 0){
    for(auto it=case_similarities.at(test_case).begin(); it!=case_similarities.at(test_case).end(); ++it){
      if(ranked_cases.count(*it) != 0){
	visit_test_case(*it);
	break;
      }
    }
  }
}

void calculate_case_order(){
  int current_case;
  while(!ranked_cases.empty()){
    current_case = *(ranked_cases.begin());
    visit_test_case(current_case);
  }
}

float calculate_case_distance(int case1, int case2){ 
  if(bbl_universe_size == 0){
    return 0.0;
  }
  float dist = (float) (bbl_vector[case1] ^ bbl_vector[case2]).count() / (float) bbl_universe_size;
  return dist;
}

void distance_task(int case1, int case2){
  if(case1 == case2)
    throw range_error("Tried to calculate distance between the same test case.");
  if(case1 > case2)
    throw range_error("Tried to calculate an already calculated distance.");
  
  float dist = calculate_case_distance(case1, case2);
  case_distances[case1][case2-case1-1] = dist;
  distances_number++;
  distances_avg+=dist;

  if(dist < threshold){
    distances_below_threshold++;
    case_ranks[case2]++;
    case_similarities.at(case2).insert(case1);
    case_ranks[case1]++;
    case_similarities.at(case1).insert(case2);
  }
}

void calculate_case_distances(){
  for(int i=0; i<test_number; i++){
    for(int j=i+1; j<test_number; j++){
      distance_task(i, j);
    }
    ranked_cases.insert(i);
  }
}

void init_data_structures(){
  int i,j;
  for(i=0; i<test_number; i++){
    vector<float> vec;
    for(j=1; j<test_number-i; j++)
      vec.push_back(0.0);
    case_distances.push_back(vec);

    case_ranks.push_back(0);

    distance_operator dop(i);
    set<int, distance_operator> similarity_set(dop);
    case_similarities.push_back(similarity_set);
  }
}

void calculate_threshold(){
  cache_instruction_capacity = (float) l1_ins_cache_size / (float) average_ins_size;
  
  threshold = 1.0 - (average_instructions_executed / cache_instruction_capacity);

  if(threshold < 0){
    threshold = cache_instruction_capacity / average_instructions_executed;
  }

  if(threshold < low_threshold){
    threshold = low_threshold;
  }
}

void init(){
  test_number = bbl_vector.size();
  calculate_threshold();
  init_data_structures();
}

int main(int argc, char *argv[]){
  if(argc < 5){
    cerr<<"Not enough arguments provided."<<endl;
    cerr<<"Arguments are:"<<endl;
    cerr<<"\t1) The average instruction size in bytes."<<endl;
    cerr<<"\t2) The L1 instruction cache size in bytes."<<endl;
    cerr<<"\t3) The case order file."<<endl;
    cerr<<"\t4) The optimization details file."<<endl;
    cerr<<"\t5) The case distance threshold (optional)."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }
  
  if(!isdigit(argv[1][0])){
    cerr<<"Average instruction size must be an integer above zero."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }
  average_ins_size = atoi(argv[1]);
  if(average_ins_size <= 0){
    cerr<<"Average instruction size must be above zero."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }

  if(!isdigit(argv[2][0])){
    cerr<<"L1 instruction cache size must be an integer above zero."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }
  l1_ins_cache_size = atoi(argv[2]);
  if(l1_ins_cache_size <= 0){
    cerr<<"L1 instruction cache size must be above zero."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }

  fvector.open(argv[3], ios::in | ios::binary);
  if(!fvector.is_open()){
    cerr<<"Error opening binary vector file."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }

  forder.open(argv[4], ios::out);
  if(!forder.is_open()){
    cerr<<"Error opening case order file."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }

  foptdetails.open(argv[5], ios::out);
  if(!foptdetails.is_open()){
    cerr<<"Error opening optimization details file."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
    }

  {
    cereal::BinaryInputArchive iarchive(fvector);
    iarchive(bbl_vector);
  }

  init();

  calculate_case_distances();

  calculate_case_order();

  export_case_order();

  export_optimization_details();

  cout<<SUCCESS_MSG<<endl;
  return 0;
}
