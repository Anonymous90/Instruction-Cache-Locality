#include "tso.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <bitset>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/bitset.hpp>

using namespace std;

const size_t bbl_universe_size = BBLUNIV;

vector< bitset<bbl_universe_size> > bbl_vector;
bitset<bbl_universe_size> visited_bbls;
vector<int> case_order;
map<int, int> case_ranks;

int pick_best();

void calculate_ranks();

ifstream fvector;
ofstream forder;

const std::string SUCCESS_MSG = "Branch optimization completed successfully.";
const std::string ERROR_MSG = "Branch optimization failed. Exiting.";

void export_order(){
  for(int& case_id : case_order){
    forder<<case_id;
    forder<<endl;
  }
  forder.close();
}

void calculate_ranks(){
  for(auto it=case_ranks.begin(); it!=case_ranks.end(); ++it){
    case_ranks[it->first] = ((~visited_bbls) & bbl_vector[it->first]).count();
  }
}

int pick_best(int last){
  if(visited_bbls.count() == visited_bbls.size()){
    int indexDist=-1;
    int bestDist=-1;
    for(auto itd=case_ranks.begin(); itd!=case_ranks.end(); ++itd){
      int dist = (bbl_vector[last] ^ bbl_vector[itd->first]).count();
      if(dist > bestDist){
	indexDist = itd->first;
	bestDist = dist;
      }
    }
    return indexDist;
  }

  int index=-1;
  int best=-1;
  for(auto it=case_ranks.begin(); it!=case_ranks.end(); ++it){
    if(it->second > best){
      index = it->first;
      best = it->second;
    }
  }
  return index;
}

void calculate_order(){
  int last = -1;
  while(!case_ranks.empty()){
    int best = pick_best(last);
    case_ranks.erase(best);
    case_order.push_back(best);
    visited_bbls |= bbl_vector[best];
    calculate_ranks();
    last = best;
  }
}

void init(){
  for(int i=0; i<bbl_vector.size(); i++){
    case_ranks.insert(pair<int, int>(i, bbl_vector[i].count()));
  }
}

int main(int argc, char *argv[]){
  if(argc < 3){
    cerr<<"Not enough arguments provided."<<endl;
    cerr<<"Arguments are:"<<endl;
    cerr<<"\t1) The basic block vector file."<<endl;
    cerr<<"\t2) The branch case order file."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }
  

  fvector.open(argv[1], ios::in | ios::binary);
  if(!fvector.is_open()){
    cerr<<"Error opening binary vector file."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }

  forder.open(argv[2], ios::out);
  if(!forder.is_open()){
    cerr<<"Error opening branch case order file."<<endl;
    cerr<<ERROR_MSG<<endl;
    return -1;
  }

  {
    cereal::BinaryInputArchive iarchive(fvector);
    iarchive(bbl_vector);
  }

  init();

  calculate_order();

  export_order();

  cout<<SUCCESS_MSG<<endl;
  return 0;
}
