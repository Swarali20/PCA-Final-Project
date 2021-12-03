#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <omp.h>
#include <time.h>

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

    if (argc != 3) {
        cout << "Provide path to file and thread count\n";
        exit(1);
    }

    char *graph_filename = argv[1];
    int num_threads = atoi(argv[2]);

    // vector <Page> pages;
    map <int, Page> pages;
    map <int, int> lookup;
    // string line;
    int from_idx, to_idx;
    FILE *fid;
    fid = fopen(graph_filename, "r");
    if (fid == NULL){
      printf("Error opening data file\n");
   }

    int num_pages = 0;
   while (!feof(fid)) {
      if (fscanf(fid,"%d,%d\n", &from_idx,&to_idx)) {
          if (!pages.count(from_idx)) {
              pages[from_idx] = Page();
              pages[from_idx].num_in_pages = 0;
              pages[from_idx].num_out_pages = 0;
              pages[from_idx].page_rank = 0;
              num_pages++;
          }
          if (!pages.count(to_idx)) {
              pages[to_idx] = Page();
              pages[to_idx].num_in_pages = 0;
              pages[to_idx].num_out_pages = 0;
              pages[to_idx].page_rank = 0;
              num_pages++;
          }

          pages[from_idx].num_out_pages++;
          pages[to_idx].num_in_pages++;
          pages[to_idx].incoming_ids.push_back(from_idx);
        
      }
   }
   fclose(fid);

}