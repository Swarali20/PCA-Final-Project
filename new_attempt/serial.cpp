#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <time.h>
#include <math.h>
#include <algorithm>

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
      cout << i << "Is dangling" << endl;
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
      // cout << iter - pages.begin() << endl;
      return (iter - pages.begin());
   }else{
    //  cout << -1 << endl;
    return -1;
   }
}

int main(int argc, char** argv){

    if (argc != 3) {
        cout << "Provide path to file and thread count\n";
        exit(1);
    }

    char *graph_filename = argv[1];
    int damping = atoi(argv[2]);

    std::vector<Page> pages;
    std::vector<int> out_link_cnts;

    int from_idx, to_idx;
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
        
        
        from_idx_pos = find_element(pages, from_idx);
        to_idx_pos = find_element(pages, to_idx);

        if ( from_idx_pos < 0){
          cout << "New page for: " << from_idx << endl;
          pages.push_back(Page());
          pages[num_pages].ID = from_idx;
          num_pages++;
        }
        if (to_idx_pos < 0){
          cout << "New page for: " << to_idx << endl;
          pages.push_back(Page());
          pages[num_pages].ID = to_idx;
          num_pages++;
        }
        from_idx_pos = find_element(pages, from_idx);
        to_idx_pos = find_element(pages, to_idx);
        // cout << "From idx pos: " << from_idx_pos << endl;
        // cout << "To idx pos: " << to_idx_pos << endl;
        pages[to_idx_pos].num_in_pages++;
        pages[to_idx_pos].incoming_ids.push_back(from_idx);
        pages[from_idx_pos].num_out_pages++;
      }
   }
   cout << "Num pages: " << num_pages<< endl;
   out_link_cnts.reserve(num_pages);
   for (int i=0; i<num_pages;i++){
     out_link_cnts.push_back(pages[i].num_out_pages);
   }
  
  // cout << "Out size: " << out_link_cnts.size() <<endl;

  cout<<"Graph data loaded in "<<(double)(clock() - tStart)/CLOCKS_PER_SEC<<"s"<<endl;
  
  std::vector<int> dangling_pages = ExploreDanglingPages(out_link_cnts);
  // cout << "size: " << dangling_pages.size();
  
  
}