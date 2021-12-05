#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
//#include <omp.h>
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
    int damping = atoi(argv[2]);

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
    float initialize_score = 1.0 / num_pages;
    for(auto& x : pages)
        x.second.page_rank = initialize_score;

    int num_iterations = 10;
    int idx;
      clock_t aStart = clock();

    for (int iter = 0; iter < num_iterations;iter++){
        for (int i = 0; i<pages.size();i++){
            cout << "Node index: " << lookup[i] << endl;
            idx = lookup[i];
            pages[idx].temp_page_rank = 0.0;
            if (pages[idx].num_in_pages>0){
                int num_incoming = pages[idx].incoming_ids.size();
                // cout << "Incoming: ";
                for (int j = 0 ; j<num_incoming;j++){

                    int in_id = pages[idx].incoming_ids[j];
                    pages[idx].temp_page_rank += pages[in_id].page_rank / pages[in_id].num_out_pages;
                }
            }
            pages[idx].page_rank = (1.0 - damping) * pages[idx].temp_page_rank + damping / num_pages;
            cout << "New page rank: " << pages[idx].page_rank << endl;
            // cout << endl;

        }
    }
      cout<<"Algorithm terminated in "<<(double)(clock() - aStart)/CLOCKS_PER_SEC<<"s"<<endl;


    fclose(fid);

}