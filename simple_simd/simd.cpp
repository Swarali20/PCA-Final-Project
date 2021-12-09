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
#include <cstdlib>
#include <unistd.h>
#include "immintrin.h"
using namespace std;



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


std::vector<float> InitPr(int page_cnt) {
	std::vector<float> pr;
	pr.reserve(page_cnt);
	float init_pr = 1.0/page_cnt;
	for (int i = 0; i < page_cnt; i++) {
		pr.push_back(init_pr);
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
		std::vector<double> out_link_cnts_rcp,
		std::vector<float> &old_pr,
		std::vector<float> &new_pr) {

	for (int i = 0; i < pages.size(); i++) {

		double sum = 0;
		int  num_incoming = pages[i].size_incoming_ids;

		for (int j = 0 ; j < (num_incoming/4)*4; j+= 4){

			int in_id1 = pages[i].incoming_ids[j]; 
			int in_id2 = pages[i].incoming_ids[j+1]; 
			int in_id3 = pages[i].incoming_ids[j + 2]; 
			int in_id4 = pages[i].incoming_ids[j+3]; 
			sum = sum +  (old_pr[in_id1] * out_link_cnts_rcp[in_id1]) + 
				(old_pr[in_id2] * out_link_cnts_rcp[in_id2]) + (old_pr[in_id3] * out_link_cnts_rcp[in_id3]) 
				+  (old_pr[in_id4] * out_link_cnts_rcp[in_id4]);

		}
		for (int j = (num_incoming/4)*4; j < num_incoming; j++){
			int in_id1 = pages[i].incoming_ids[j];
			sum = sum +  (old_pr[in_id1] * out_link_cnts_rcp[in_id1]);
		}
		new_pr[i] = sum;
	}
}



void AddDanglingPagesPr(
		std::vector<int> &dangling_pages,
		std::vector<float> &old_pr,
		std::vector<float> &new_pr) {
	double sum = 0;

	int dangling_pages_size = dangling_pages.size();
	for (int i = 0; i < dangling_pages_size; i++) {
		sum += old_pr[dangling_pages[i]];
	}

	int new_pr_size = new_pr.size();
	double val = sum/new_pr_size;
	__m256 val_buf = _mm256_set1_ps(val);
	__m256 pr_buf, pr_buf1;
	new_pr_size = (new_pr_size/8) * 8;
	for (int i = 0; i < new_pr_size; i+= 8) {
		pr_buf = _mm256_loadu_ps(&(new_pr[i]));
		pr_buf1 = _mm256_add_ps(pr_buf, val_buf);
		_mm256_storeu_ps(&(new_pr[i]), pr_buf1);
	}

}

void AddRandomJumpsPr(
		double damping_factor,
		std::vector<float> &new_pr) {
	int new_pr_size = new_pr.size();
	double val = (1- damping_factor) * 1.0 /new_pr_size;
	__m256 val_buf = _mm256_set1_ps(val);
	__m256 damping_buf = _mm256_set1_ps(damping_factor);
	__m256 pr_buf, pr_buf1;

	for (int i = 0; i < (new_pr_size/8) * 8; i+=8) {
		pr_buf = _mm256_loadu_ps(&new_pr[i]);
		pr_buf1 = _mm256_fmadd_ps(pr_buf, damping_buf, val_buf);
		_mm256_storeu_ps(&(new_pr[i]),pr_buf1);
	}
	for (int i = (new_pr_size/8) * 8; i < new_pr_size; i++){
		new_pr[i] = new_pr[i] * damping_factor + (1 - damping_factor) * val;
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

	int opt = 0;
	

	char *graph_filename = NULL;
	int num_threads = 1;
  float damping = 0.85;
  do {
		opt = getopt(argc, argv, "f:d:n:");
		switch (opt) {
			case 'f':
				graph_filename = optarg;
				break;

			case 'd':
				damping = atof(optarg);
				break;

      case 'n':
      num_threads = atoi(optarg);
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
	std::vector<double> out_link_cnts_rcp;

	int from_idx, to_idx;
	FILE *fid;
	fid = fopen(graph_filename, "r");
	if (fid == NULL){
		printf("Error opening data file\n");
	}

	int num_pages = 0;
	int from_idx_pos, to_idx_pos;;

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
		if (input_pages[idx].num_out_pages != 0) {
			out_link_cnts_rcp.push_back(1.0/input_pages[idx].num_out_pages);

		} else {
			out_link_cnts_rcp.push_back(0);
		}

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
		pages[i].size_incoming_ids = pages[i].incoming_ids.size();

	}
	load_time += duration_cast<dsec>(Clock::now() - load_start).count();
	printf("Graph data loaded in: %lf.\n", load_time);

	std::vector<int> dangling_pages = ExploreDanglingPages(out_link_cnts);
	std::vector<float> pr = InitPr(num_pages);
	std::vector<float> old_pr(num_pages);

	auto compute_start = Clock::now();
	double compute_time = 0;
	// double addPage_time = 0;
	// double other_compute = 0;
	for (int iter = 0; iter < 80; iter++){

		std::copy(pr.begin(), pr.end(), old_pr.begin());

		// auto addPage_start = Clock::now();
		AddPagesPr(pages, out_link_cnts_rcp, old_pr, pr);

		// addPage_time += duration_cast<dsec>(Clock::now() - addPage_start).count();

		// auto other_compute_start = Clock::now();
		AddDanglingPagesPr(dangling_pages, old_pr, pr);

		AddRandomJumpsPr(0.85, pr);

		// other_compute += duration_cast<dsec>(Clock::now() - other_compute_start).count();

	}
	compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
	printf("Computation Time: %lf.\n", compute_time);

	// for (auto i: pr)
	//   std::cout << i << ' '; 

	// cout << endl;   
}
