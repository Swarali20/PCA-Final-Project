#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <chrono>
#include <time.h>
#include <cstdlib>
#include <unistd.h>

using namespace std;

struct Page {
	vector <int> incoming_ids;
	int num_in_pages;
	int num_out_pages;
	float page_rank;
	float temp_page_rank;
};

void Read_from_txt_file(char * filename)
{

	FILE *fid;
	int from_idx, to_idx;
	int temp_size;

	fid = fopen(filename, "r");
	if (fid == NULL){
		printf("Error opening data file\n");
	}

	while (!feof(fid)) {
		if (fscanf(fid,"%d,%d\n", &from_idx,&to_idx)) {
			cout << from_idx << " - " << to_idx << endl;
		}
	}
	fclose(fid);
}

int main(int argc, char** argv){
	using namespace std::chrono;
	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::duration<double> dsec;

	int opt = 0;
	char *graph_filename = NULL;
	int num_threads = 1;
	float damping = 0.0f;
	do {
		opt = getopt(argc, argv, "f:d");
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



	map <int, Page> pages;
	map <int, int> lookup;
	int from_idx, to_idx;
	FILE *fid;
	fid = fopen(graph_filename, "r");
	if (fid == NULL){
		printf("Error opening data file\n");
	}

	int num_pages = 0;
	auto load_start = Clock::now();
	double load_time = 0;
	while (!feof(fid)) {
		if (fscanf(fid,"%d,%d\n", &from_idx,&to_idx)) {
			if (!pages.count(from_idx)) {
				pages[from_idx] = Page();
				pages[from_idx].num_in_pages = 0;
				pages[from_idx].num_out_pages = 0;
				pages[from_idx].page_rank = 0;
				lookup[num_pages] = from_idx;
				num_pages++;
			}
			if (!pages.count(to_idx)) {
				pages[to_idx] = Page();
				pages[to_idx].num_in_pages = 0;
				pages[to_idx].num_out_pages = 0;
				pages[to_idx].page_rank = 0;
				lookup[num_pages] = to_idx;
				num_pages++;
			}

			pages[from_idx].num_out_pages++;
			pages[to_idx].num_in_pages++;
			pages[to_idx].incoming_ids.push_back(from_idx);
		}
	}
	fclose(fid);
	load_time += duration_cast<dsec>(Clock::now() - load_start).count();
	printf("Graph data loaded in: %lf.\n", load_time);

	auto compute_start = Clock::now();
	double compute_time = 0;
	float initialize_score = 1.0 / num_pages;
	for(auto& x : pages)
		x.second.page_rank = initialize_score;

	int num_iterations = 5;
	int idx;
	for (int iter = 0; iter < num_iterations;iter++){
		for (int i = 0; i<pages.size();i++){
			idx = lookup[i];
			pages[idx].temp_page_rank = 0.0;
			if (pages[idx].num_in_pages > 0){
				int num_incoming = pages[idx].incoming_ids.size();
				for (int j = 0 ; j<num_incoming;j++){
					int in_id = pages[idx].incoming_ids[j];
					pages[idx].temp_page_rank += pages[in_id].page_rank / pages[in_id].num_out_pages;
				}
			}
			pages[idx].page_rank = (1.0 - damping) * pages[idx].temp_page_rank + damping / num_pages;
		}
	}
	compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
	printf("Computation Time: %lf.\n", compute_time);

}
