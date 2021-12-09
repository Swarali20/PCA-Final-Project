#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <time.h>
#include <math.h>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <chrono>

using namespace std;

struct Page {
    int ID;
    vector <int> incoming_ids;
    int num_in_pages;
    int num_out_pages;
    long double page_rank;
    long double temp_page_rank;
};


std::vector<int> ExploreDanglingPages(std::vector<int> &out_link_cnts) {
	std::vector<int> dangling_pages;
	for (int i = 0; i < out_link_cnts.size(); i++) {
		if (out_link_cnts[i] == 0) {
			dangling_pages.push_back(i);
		}
	}
	return dangling_pages;
}


std::vector<long double> InitPr(int page_cnt) {
	std::vector<long double> pr;
	pr.reserve(page_cnt);
	for (int i = 0; i < page_cnt; i++) {
		pr.push_back(1.0 / page_cnt);
	}
	return pr;
}

int find_element(const std::vector<Page> &pages, int ID){

   auto iter = std::find_if(pages.begin(), pages.end(), 
               [&](const Page& p){return p.ID == ID;});
   
   if (iter != pages.end()){
      return (iter - pages.begin());
   }else{
    return -1;
   }
}

void AddPagesPr(
		std::vector<Page> &pages,
		std::vector<int> &out_link_cnts,
		std::vector<long double> &old_pr,
		std::vector<long double> &new_pr) {
	for (int i = 0; i < pages.size(); i++) {
		long double sum = 0;
    if (pages[i].num_in_pages>0){
      int num_incoming = pages[i].incoming_ids.size();
      for (int j = 0 ; j<num_incoming;j++){
          int in_id = pages[i].incoming_ids[j];
          sum += old_pr[in_id] / out_link_cnts[in_id];
      }
    }
		new_pr[i] = sum;
	}
}

void AddDanglingPagesPr(
		std::vector<int> &dangling_pages,
		std::vector<long double> &old_pr,
		std::vector<long double> &new_pr) {
	long double sum = 0;
	for (int page : dangling_pages) {
		sum += old_pr[page];
	}
	for (long double &pr : new_pr) {
		pr += sum / new_pr.size();
	}
}

void AddRandomJumpsPr(
		float damping_factor,
		std::vector<long double> &new_pr) {
	for (long double &pr : new_pr) {
		pr = pr * damping_factor + (1 - damping_factor) / new_pr.size();
	}
}

int main(int argc, char** argv){

    using namespace std::chrono;
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::duration<double> dsec;

  int opt = 0;
  float damping = 0.85f;
  char *graph_filename = NULL;
   do {
    opt = getopt(argc, argv, "f:n:i:");
    switch (opt) {
    case 'f':
      graph_filename = optarg;
      break;

    case 'd':
      damping = atof(optarg);
      break;
    case -1:
      break;

    default:
      break;
    }
  } while (opt != -1);

    map <int, Page> input_pages;
    map <int, int> lookup;
    map<int, int > rev_lookup;

    std::vector<Page> pages;
    std::vector<int> out_link_cnts;

    int from_idx, to_idx;
    auto load_start = Clock::now();
    double load_time = 0;
    FILE *fid;
    fid = fopen(graph_filename, "r");
    if (fid == NULL){
      printf("Error opening data file\n");
   }

    int num_pages = 0;
    int from_idx_pos, to_idx_pos;;
    clock_t tStart = clock();

    while (!feof(fid)) {
      if (fscanf(fid,"%d,%d\n", &from_idx,&to_idx)) {
        
        if (!input_pages.count(from_idx)) {
              input_pages[from_idx] = Page();
              input_pages[from_idx].num_in_pages = 0;
              input_pages[from_idx].num_out_pages = 0;
              input_pages[from_idx].page_rank = 0;
              lookup[num_pages] = from_idx;
              rev_lookup[from_idx] = num_pages;
              num_pages++;
          }
          if (!input_pages.count(to_idx)) {
              input_pages[to_idx] = Page();
              input_pages[to_idx].num_in_pages = 0;
              input_pages[to_idx].num_out_pages = 0;
              input_pages[to_idx].page_rank = 0;
              lookup[num_pages] = to_idx;
              rev_lookup[to_idx] = num_pages;
              num_pages++;
          }

          input_pages[from_idx].num_out_pages++;
          input_pages[to_idx].num_in_pages++;
          input_pages[to_idx].incoming_ids.push_back(from_idx);
      }
   }


   cout << "Num pages: " << num_pages<< endl;
   out_link_cnts.reserve(num_pages);
    
    int idx;
   for (int i=0; i<num_pages;i++){
     idx = lookup[i]; 
     out_link_cnts.push_back(input_pages[idx].num_out_pages);
   }

  int vec_idx;
  for (int i=0; i<num_pages;i++){
    pages.push_back(Page());

    idx = lookup[i];
   
    pages[i].num_in_pages = input_pages[idx].num_in_pages;
    pages[i].ID = idx;
    
    for (int j=0;j<input_pages[idx].incoming_ids.size();j++){
      
      vec_idx = rev_lookup[input_pages[idx].incoming_ids[j]];
      pages[i].incoming_ids.push_back(vec_idx);
    
    }  
  }

load_time += duration_cast<dsec>(Clock::now() - load_start).count();
  printf("Graph data loaded in: %lf.\n", load_time);

  std::vector<int> dangling_pages = ExploreDanglingPages(out_link_cnts);
  std::vector<long double> pr = InitPr(num_pages);
	std::vector<long double> old_pr(num_pages);


  auto compute_start = Clock::now();
  double compute_time = 0;
  for (int iter = 0; iter < 80; iter++){

		std::copy(pr.begin(), pr.end(), old_pr.begin());

		AddPagesPr(pages, out_link_cnts, old_pr, pr);
    AddDanglingPagesPr(dangling_pages, old_pr, pr);
		AddRandomJumpsPr(damping, pr);

  }

 compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
  printf("Computation Time: %lf.\n", compute_time);
  // for (auto i: pr)
  //   std::cout << i << ' ';    
}