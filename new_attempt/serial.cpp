#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <time.h>
#include <math.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <omp.h>
#include <chrono>
#include "immintrin.h"
using namespace std;

//macro intrinsics for selected instruction
#define INSTRUCTION(dest, src)     \
   __asm__ __volatile__(           \
       "vaddpd %[rsrc1], %[rsrc2], %[rdest]\n"   \
   : [rdest] "+x"(dest)            \
   : [rsrc1] "x"(dest), [rsrc2] "x"(src));

struct Page {
    int ID;
    vector <int> incoming_ids;
    int size_incoming_ids;
    int num_in_pages;
    int num_out_pages;
    double page_rank;
    double temp_page_rank;
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


std::vector<double> InitPr(int page_cnt) {
	std::vector<double> pr;
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
      // cout << iter - pages.begin() << endl;
      return (iter - pages.begin());
   }else{
    //  cout << -1 << endl;
    return -1;
   }
}

void AddPagesPr(
		std::vector<Page> &pages,
		// std::vector<double> out_link_cnts_rcp,
    std::vector<int> out_link_cnts,
		std::vector<double> &old_pr,
		std::vector<double> &new_pr) {

  // int pr[pages.size()];
  // int chunk_size = 8;
  #pragma omp parallel for schedule(dynamic, 128)
  // #pragma omp simd
	for (int i = 0; i < pages.size(); i++) {

    // float buffer_old_pr[8];
		double sum = 0;
    // int  num_incoming = pages[i].size_incoming_ids;
    int num_incoming = pages[i].incoming_ids.size();
    // #pragma omp parallel for reduction(+:sum)

    for (int j = 0 ; j < num_incoming; j++){
       
        int in_id = pages[i].incoming_ids[j]; 
        // sum += (old_pr[in_id] * out_link_cnts_rcp[in_id]);
        sum += (old_pr[in_id] / out_link_cnts[in_id]);
    }
    new_pr[i] = sum;
	}

  // std::copy(pr.begin(), pr.end(), new_pr.begin());
}



void AddDanglingPagesPr(
		std::vector<int> &dangling_pages,
    // std::vector<int> &dangling_pangle,
		std::vector<double> &old_pr,
		std::vector<double> &new_pr) {
	double sum = 0;
  // #pragma omp parallel for reduction (+:sum)
  for (int i = 0; i < dangling_pages.size(); i++) {
    sum += old_pr[dangling_pages[i]];
  }
	// for (int page : dangling_pages) {
	// 	sum += old_pr[page];
	// }
  int new_pr_size = new_pr.size();
  double val = sum/new_pr_size;
  // __m256d val_buf = _mm256_broadcast_sd(&val);
  // __m256d pr_buf;
  // // #pragma omp parallel for 
  // new_pr_size = (new_pr_size/4) * 4;
  for (int i = 0; i < new_pr_size; i+= 4) {
    // pr_buf = _mm256_loadu_pd(&(new_pr[i]));
    // //INSTRUCTION(pr_buf, val_buf);
    // pr_buf = _mm256_add_pd(pr_buf, val_buf);
    new_pr[i] += val;
    // _mm256_storeu_pd(&(new_pr[i]), pr_buf);
  }

	// for (double &pr : new_pr) {
	// 	pr += sum / new_pr.size();
	// }
}

void AddRandomJumpsPr(
		double damping_factor,
		std::vector<double> &new_pr) {
    int new_pr_size = new_pr.size();
    // double val = 1/new_pr_size;
	// for (double &pr : new_pr) {
  #pragma omp parallel for schedule(dynamic, 128)
  for (int i = 0; i < new_pr_size; i++) {
		// new_pr[i] = new_pr[i] * damping_factor + (1 - damping_factor) *val;
    new_pr[i] = new_pr[i] * damping_factor + (1 - damping_factor) /new_pr_size;
	}
}

double L1Norm(
		const std::vector<double> a,
		const std::vector<double> b) {
	double sum = 0;
	for (int i = 0; i < a.size(); i++) {
		sum += std::abs(a[i] - b[i]);
	}
	return sum;
}

int main(int argc, char** argv){
  using namespace std::chrono;
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::duration<double> dsec;

    if (argc != 3) {
        cout << "Provide path to file and thread count\n";
        exit(1);
    }

    char *graph_filename = argv[1];
    int num_threads = atoi(argv[2]);
    omp_set_num_threads(num_threads);

    map <int, Page> input_pages;
    map <int, int> lookup;
    map<int, int > rev_lookup;

    std::vector<Page> pages;
    std::vector<int> out_link_cnts;
    std::vector<double> out_link_cnts_rcp;

    int from_idx, to_idx;
    FILE *fid;
    fid = fopen(graph_filename, "r");
    if (fid == NULL){
      printf("Error opening data file\n");
   }

    int num_pages = 0;
    int from_idx_pos, to_idx_pos;;
    // clock_t tStart = clock();
    auto load_start = Clock::now();
    double load_time = 0;
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
   out_link_cnts_rcp.reserve(num_pages);
    
    int idx;
   for (int i=0; i<num_pages;i++){
     idx = lookup[i]; 
     
     out_link_cnts.push_back(input_pages[idx].num_out_pages);
     if (input_pages[idx].num_out_pages != 0)
      out_link_cnts_rcp.push_back(1/input_pages[idx].num_out_pages);
    else 
      out_link_cnts_rcp.push_back(0);
   }

  //  for (auto &i : out_link_cnts){
  //    cout << i << endl;
  //  }

  // cout << "~~~~~~~";
  int vec_idx;
  for (int i=0; i<num_pages;i++){
    pages.push_back(Page());

    idx = lookup[i];
    // cout << i << " " << input_pages[idx].num_in_pages << " " << input_pages[idx].num_out_pages <<" " << endl;
    pages[i].num_in_pages = input_pages[idx].num_in_pages;
    pages[i].ID = idx;
    
    
    // cout << "Incoming ID's for: " << i << "/" << idx << endl;
    for (int j=0;j<input_pages[idx].incoming_ids.size();j++){
      
      // cout << input_pages[idx].incoming_ids[j] << " ";
      vec_idx = rev_lookup[input_pages[idx].incoming_ids[j]];
      // cout << "Vector ID: " << vec_idx << endl;
      pages[i].incoming_ids.push_back(vec_idx);
    
    } 
    pages[i].size_incoming_ids = input_pages[idx].incoming_ids.size();
    // cout << endl; 
  }

  // cout << "Out size: " << out_link_cnts.size() <<endl;

  // cout<<"Graph data loaded in "<<(double)(clock() - tStart)/CLOCKS_PER_SEC<<"s"<<endl;
  load_time += duration_cast<dsec>(Clock::now() - load_start).count();
  printf("Graph data loaded in: %lf.\n", load_time);

  std::vector<int> dangling_pages = ExploreDanglingPages(out_link_cnts);
  std::vector<double> pr = InitPr(num_pages);
	std::vector<double> old_pr(num_pages);


  bool go_on = true;
	int step = 0;
	// while (go_on) {
  
  // clock_t aStart = clock();
  auto compute_start = Clock::now();
  double compute_time = 0;
  double addPage_time = 0;
  double other_compute = 0;
  for (int iter = 0; iter < 80; iter++){

		std::copy(pr.begin(), pr.end(), old_pr.begin());
    auto addPage_start = Clock::now();
		AddPagesPr(pages, out_link_cnts, old_pr, pr);
    addPage_time += duration_cast<dsec>(Clock::now() - addPage_start).count();
    // cout << iter << " ";
    // printf("AddPage Time: %lf.\n", addPage_time);

    // cout << endl;
    auto other_compute_start = Clock::now();
    AddDanglingPagesPr(dangling_pages, old_pr, pr);
		AddRandomJumpsPr(0.85, pr);
    other_compute += duration_cast<dsec>(Clock::now() - other_compute_start).count();
    
    // double err = L1Norm(pr, old_pr);
    // cout << "Error: " << std::setprecision(10) << std::fixed << err << endl;

  }
 
  // cout<<"Algorithm terminated in "<<(double)(clock() - aStart)/CLOCKS_PER_SEC<<"s"<<endl;
  compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
  printf("Computation Time: %lf.\n", compute_time);
   printf("AddPage Time: %lf.\n", addPage_time);
  printf("other_compute Time: %lf.\n", other_compute);
  
  // for (auto i: pr)
  //   std::cout << i << ' '; 

  // cout << endl;   
}