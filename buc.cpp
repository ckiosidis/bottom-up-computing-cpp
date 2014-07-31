#include <iostream> 
#include <sstream> 
#include <fstream> 
#include <stdio.h>
#include <string>
#include <vector>
#include <math.h>
#include <sys/time.h>

#define MAX_LINE_LENGTH 4096

using namespace std;

/* types related to the output record */
typedef vector< int > V1;
typedef vector< V1 > V2;
typedef vector< V2 > V3;

typedef vector< bool > C1;
typedef vector< C1 > C2;

/* prototypes */
int** read_datafile(int**, char*);
void display_data(int**, int, int);
int find_cardinality(int**,int, int);
void buc(int**, int, int, string);
int** partition(int**, int, int, int*, int);
void write_ancestors_one(string, int*);
void init_output();
void calculate_name(string);
void subtree_name(string, vector< string >);
int output_indexof(string);
void display_output_summary();
void commit_output();
int stoi(const string&);
string itos(int);
void elapsed_time(timeval*);

void drill_down_function();
void drill_up_function();
void slice();

/* global vars */
int minsup;
int *n_dimensions;
int *n_tuples;
V3 output;
vector< string > output_names;
timeval *prev_time;

C2 bitmap_index;

int main(int argc, char **argv)
{
	if (argc != 3) {
		cout << "buc <datafile> <minsup>" << endl;
		exit(1);
	}

	cout << "Initializing";

	// mark start time
	//	prev_ = new timeval();
	//	gettimeofday(prev_time, NULL);

	int **data = NULL;

	/* allocate memory and init global vars */
	n_tuples = new int(0);
	n_dimensions = new int(0);
	minsup = atoi(argv[2]);

	/* read data file and get the base cuboid */
	data = read_datafile(data, argv[1]);

	/* setup output record */
	init_output();

	/* start the BUC */
	buc(data, 0, *n_tuples, "");

	/* display output and save to disk */
	display_output_summary();
	commit_output();
    *n_dimensions=6;
	/* clean up */
	//	delete n_dimensions;
	//delete n_tuples;
	//int int1;
	//    scanf("Please %d",&int1);
	//	cin >>  int1;
	

    int decision=1;
    while(decision!=0){
          cout << "Choose function to perform" << endl;
          cout << "(1) drill down " << endl;
          cout << "(2) roll up " << endl;
          cout << "(3) slice " << endl;
          cout << "(0) exit " << endl;
          cin >> decision;
          if( decision==0 ) 
            cout << " Bye bye";
          else if(decision==2)
			drill_up_function();
          else if(decision==3)
			slice();
          else if(decision==1)
			drill_down_function();
                        
      }
	return 0;
}

void buc(int **data, int dim, int data_dim_size, string path)
{
	/* optimization.  if data size is 1, no need to compute.  set all
	 * ancestor's count to 1 */
	if (data_dim_size == 1) {
		write_ancestors_one(path, data[0]);
		return;
	}

	int **frequency;
	frequency = new int*[*n_dimensions];

	/* write the count to the output record */
	if ((data_dim_size > 0) & (path.length() > 0)) {
		vector< int > v;
		int temp;
		istringstream is_path(path);

		/* while there are still more tokens in the path */
		while (is_path >> temp) {
			v.push_back(data[0][temp]);
		}
		v.push_back(data_dim_size);

		int output_index = output_indexof(path);
		output[output_index].push_back(v);
	}

	for (int d = dim; d < *n_dimensions; d++) {
		/* attach the current cuboid name onto the path */
		if (path.length() == 0) { path = itos(d) + " "; }
		else { path += itos(d) + " "; }

		/* find cardinality of data on dimension d */
		int C = find_cardinality(data, d, data_dim_size);

		/* partition the data on dimension d.  also calculate the
		 * frequency of each number (dataCount in the paper) */
		frequency[d] = new int[C];
		data = partition(data, d, C, frequency[d], data_dim_size);

		/* for each partition */
		for (int i = 0; i < C; i++) {
			int c = frequency[d][i];

			/* the BUC ends here if minsup is not satisfied */
			if (c >= minsup) {
				/* construct new data set that is a subset of the
				 * original data.  this is a partition. */
				int** sub_data = new int*[c];

				/* figure out where in the original data does the
				 * particular partition start */
				int c_start = 0;
				for (int j = 0; j < i; j++) {
					c_start += frequency[d][j];
				}

				/* copy the values of the data into the new subset.
				 * note this is just a copy of the pointers.  they're
				 * still using the same actual data */
				for (int j = 0; j < c; j++) {
					sub_data[j] = data[c_start + j];
				}

				/* recursively call buc with partition */
				buc(sub_data, d + 1, c, path); 
			}
		}

		/* remove the last cuboid name in the path */
		if (path.length() == 2) { path = ""; }
		else { 
			if (path[path.length() - 3] == ' ') {
				path = path.substr(0, path.length() - 2); 
			}
			else {
				path = path.substr(0, path.length() - 3); 
			}
		}
	}


	/* clean up */
	for (int i = dim; i < *n_dimensions; i++) { delete[] frequency[i]; }
	delete[] frequency;
	if (path == "") {
		/* this completely removes the input data from memory.  it is
		 * called when the entire BUC algorith ends */
		for (int i = 0; i < *n_tuples; i++) { delete[] data[i]; }
	}
	delete[] data;
}

int** partition(int **data, int dimension, int cardinality, int
		*frequency, int data_dim_size)
{
	int *counting_sort_freq = new int[cardinality];

	/* clear frequency */
	for (int i = 0; i < cardinality; i++) { frequency[i] = 0; }

	/* calculate frequency */
	for (int i = 0; i < data_dim_size; i++) {
		frequency[data[i][dimension]]++; 
	}

	/* make copy of frequency array for counting sort */
	counting_sort_freq = new int[cardinality];
	for (int i = 0; i < cardinality; i++) { 
		counting_sort_freq[i] = frequency[i];
	}

	/* add previous frequency count to current.  i.e.
	 * counting_sort_freq[i] will mean there are counting_sort_freq[i]
	 * numbers < i */
	for (int i = 1; i < cardinality; i++) { 
		counting_sort_freq[i] += counting_sort_freq[i - 1];
	}

	/* allocate space for sorted results */
	int **sorted_data = new int*[data_dim_size];

	/* counting sort */
	for (int i = 0; i < data_dim_size; i++) {
		sorted_data[counting_sort_freq[data[i][dimension]] - 1] =
			data[i];
		counting_sort_freq[data[i][dimension]]--;
	}

	/* clean up */
	delete[] data;
	delete[] counting_sort_freq;

	return sorted_data;
}

/* returns the cardinality of data on a specific dimension */
int find_cardinality(int **data, int dimension, int data_dim_size) 
{
	/* this shouldn't happen */
	if (data_dim_size == 0) { return 0; }

	int max = data[0][dimension];
	for (int i = 1; i < data_dim_size; i++) {
		if (data[i][dimension] > max) { max = data[i][dimension]; } 
	}

	/* max + 1 because i'm assuming the range is 0..max.  this is
	 * unnecessary in this assignment because the generated data has
	 * range 1..max. */
	return (max + 1);
}

int** read_datafile(int **data, char *filename) 
{
	FILE *f;
	int temp, i, j;
	char line[MAX_LINE_LENGTH], *word;

	// open data file
	if ((f = fopen(filename, "r")) == NULL) {
		printf("Error: cannot open file %s.\n", filename);
		exit(-1);
	}
/*
	// read first line of data file 
	fgets(line, MAX_LINE_LENGTH, f);

	// read in the number of tuples
	word = strtok(line, " ");
	*n_tuples = atoi(word);

	*n_dimensions = 0;
	word = strtok(NULL, " ");

	// count the number of dimensions
	while (word != NULL && (0 != strcmp(word, "\n"))) {
		(*n_dimensions)++;
		word = strtok(NULL, " ");
	} */

    *n_dimensions=6;
    *n_tuples=100000;
	/* create appropriate sized array to hold all data */
	data = new int*[*n_tuples];
	for (i = 0; i < *n_tuples; i++) { data[i] = new int[*n_dimensions]; }

	//	elapsed_time(prev_time);
	cout << "Reading input file";

	/* read in all data */
	i = 0;
	j = 0;

	for (i = 0; i < *n_tuples; i++) {

		// read a line
		fgets(line, MAX_LINE_LENGTH, f);

		word = strtok(line, ";");

		for (j = 0; j < *n_dimensions; j++) {
			temp = atoi(word);
			data[i][j] = temp;
			word = strtok(NULL, ";");
		}
	}
	//	elapsed_time(prev_time);
	/* done with all file reading.  close data file */
	fclose(f);

	return data;
}

void display_data(int **data, int dimension, int n_tuples)
{
	/* display data */
	for (int i = 0; i < n_tuples; i++) {
		for (int j = 0; j < dimension; j++) {
			cout << data[i][j] << " ";
		}
		cout << endl;
	}
	cout << endl;
}

/* initialize output record */
void init_output()
{
	/* initialize all the data cuboids' names.  there should be exactly
	 * 2^n_dimension of these, including the "all" cuboid. */
	output_names.push_back("ALL");
	for (int i = 0; i < *n_dimensions; i++) {
		calculate_name(itos(i) + " ");
	}

	/* resize the number of data cuboids to 2^n_dimensions */
	output.resize(output_names.size());
}

/* recursively compute the parse tree */
void calculate_name(string cd)
{
	output_names.push_back(cd);

	/* find out the last cuboid in the cd string */
	int last;
	if (cd.length() == 2) {
		last = stoi(cd.substr(cd.length() - 2, 1));
	}
	else {
		if (cd[cd.length() - 3] == ' ') {
			last = stoi(cd.substr(cd.length() - 2, 1));
		}
		else {
			last = stoi(cd.substr(cd.length() - 3, 2)); 
		}
	}

	/* see if we're on the last possible dimension */
	if (last + 1 == *n_dimensions)
		return;
	else {
		for (int i = last + 1; i < *n_dimensions; i++) {
			string next_cd = cd + itos(i) + " ";
			calculate_name(next_cd);
		}
	}
}

/* returns index of a cuboid in the ` record based on its name */
int output_indexof(string path)
{
	for (int i = 0; i < (int) pow(2.0, (double) *n_dimensions); i++) {
		if (output_names[i] == path) {
			return i;
		}
	}

	/* should never get here */
	return 0;
}

/* optimization. if current partition has size 1, this sets all
 * ancestors or more aggregated cuboids to 1 */
void write_ancestors_one(string path, int *tuple)
{
	vector< int > v;
	int temp;
	istringstream is_path(path);

	/* while there are still more tokens in the path */
	while (is_path >> temp) {
		v.push_back(tuple[temp]);
	}
	/* puts the count = 1 */
	v.push_back(1);

	int output_index = output_indexof(path);
	output[output_index].push_back(v);

	/* find out the last cuboid in the cd string */
	int last;
	if (path.length() == 2) {
		last = stoi(path.substr(path.length() - 2, 1));
	}
	else {
		if (path[path.length() - 3] == ' ') {
			last = stoi(path.substr(path.length() - 2, 1));
		}
		else {
			last = stoi(path.substr(path.length() - 3, 2)); 
		}
	}

	/* see if we're on the last possible dimension */
	if (last + 1 == *n_dimensions)
		return;
	else {
		for (int i = last + 1; i < *n_dimensions; i++) {
			string next_path = path + itos(i) + " ";
			write_ancestors_one(next_path, tuple);
		}
	}
}

void display_output_summary()
{
	for (int i = 0; i < (int) pow(2.0, (double) *n_dimensions); i++) {
		if (output[i].size() > 0) {
			int temp;
			istringstream iss(output_names[i]);
			while (iss >> temp) {
				cout << (char) (temp + 97);
			}
			cout << ":" << output[i].size() << " " << endl;
		}
	}
}

void commit_output()
{
	/* open output file */
	ofstream outfile("output", ios::out);
	if (!outfile) {
		cout << "Error opening output file" << endl;	
		exit(1);
	}

	/* write */
	for (int i = 0; i < (int) pow(2.0, (double) *n_dimensions); i++) {
		if (output[i].size() > 0) {
			int temp;
			istringstream iss(output_names[i]);
			while (iss >> temp) {
				outfile << (char) (temp + 97);
			}
			outfile << endl;
			for (unsigned int j = 0; j < output[i].size(); j++) {
				for (unsigned int k = 0; k < output[i][j].size(); k++) {
					outfile << output[i][j][k] << " ";
				}
				outfile << endl;
			}
			outfile << endl;
		}
	}
   
	outfile.close();
}

/* converts a string to an integer */
int stoi(const string &s)
{
	int result;
	istringstream(s) >> result;
	return result;
}

/* converts an integer to a string */
string itos(int n)
{
	ostringstream o;
	o << n;
	return o.str();
}

void elapsed_time(timeval *st) 
{
	timeval *et = new timeval();
	//	gettimeofday(et, NULL);
	long time = 1000 * (et->tv_sec - st->tv_sec) + (et->tv_usec -
			st->tv_usec)/1000;
	cout << " ... used time: " << time << " ms." << endl;
	memcpy(st, et, sizeof(timeval));
}



void drill_down_function()
{
     
        string temppath2="";
        cout << "Insert cuboid to perform drill down: ";
        cin>>temppath2;
        string path2="";
        int temparray[10];
        int i;

	/* open output file */
	ofstream outfile("output drill down", ios::out);
	if (!outfile) {
		cout << "Error opening output file" << endl;	
		exit(1);
	}
	
	

          
        /*transforms the string path into a numeric path 
          to find the corresponding value from output_indexof 
          eg abc -> "0 1 2 "
         */
        for( i=0; i<temppath2.length(); i++)
        {
              int temp3=temppath2[i] -97;
              path2 += itos(temp3) + " " ;
              temparray[i]=temp3;
        }


        
        

        /* write */
        int t;
        cout <<"The following cuboids are in the 'output drill down' file: "<<endl;
        for( t=temparray[i-1]+1;t<*n_dimensions;t++){
                string temppath=path2 + itos(t) + " ";
                int indexof = output_indexof(temppath);
		if (output[indexof].size() > 0) {
			int temp;
        //  cout << output_names[i]<< "!";
			istringstream iss(output_names[indexof]);
			while (iss >> temp) {
                cout << (char) (temp +97);
				outfile << (char) (temp + 97);
			}
			cout << endl;
			outfile << endl;
			for (unsigned int j = 0; j < output[indexof].size(); j++) {
				for (unsigned int k = 0; k < output[indexof][j].size(); k++) {
					outfile << output[indexof][j][k] << " ";
				}
				outfile << endl;
			}
			outfile << endl;
		}
}
   
	outfile.close();
	cout << endl;
	system("PAUSE");
	
     
}




void drill_up_function()
{


    string temppath2;
    cout << "Insert cuboid to perform roll up: ";
    cin>>temppath2;
    string path2="";
    int temparray[10];
    int i;
	/* open output file */
	ofstream outfile("output drill up", ios::out);
	if (!outfile) {
		cout << "Error opening output file" << endl;	
		exit(1);
	}

	/* write */

    /* transforms the string path into a numeric path 
       to find the corresponding value from output_indexof 
       eg abc -> "0 1 2 "
     */
    
	for( i=0; i<temppath2.length(); i++){
		int temp3=temppath2[i] -97;
		path2 += itos(temp3) + " " ;
		temparray[i]=temp3;
    }
   
    cout <<"The following cuboids are in the 'output drill up' file: "<<endl;
  
	/* find the resulting cuboids and writes them in "output drill up" file*/
        for (int j=0;j<=i-1;j++){
             string temppath;
             for(int k=0;k<=i-1;k++){
                     if(j!=k)
                     {
                            temppath+= itos(temppath2[k]-97) + " ";
                     }   
                      
                                    }
               
        
        int indexof = output_indexof(temppath);
        //cout << i<< endl;
		if (output[indexof].size() > 0) {
			int temp;
			//cout << output_names[i]<< "!";
			istringstream iss(output_names[indexof]);
			while (iss >> temp) {
                cout << (char) (temp+97);
				outfile << (char) (temp + 97);
			}
			cout << endl;
			outfile << endl;
			for (unsigned int j = 0; j < output[indexof].size(); j++) {
				for (unsigned int k = 0; k < output[indexof][j].size(); k++) {
					outfile << output[indexof][j][k] << " ";
				}
				outfile << endl;
			}
			outfile << endl;
		}
}

	outfile.close();
	cout << endl;
    system("PAUSE");
}






void slice(){

	
        string temppath2;
        cout << "Insert cuboid to perform slice: ";
        cin>>temppath2;
        string path2="";
        int temparray[10];
        int i;
     
     /* open output file */
	ofstream outfile("output slice", ios::out);
	if (!outfile) {
		cout << "Error opening output file" << endl;	
		exit(1);
	}
        
        
        /*transforms the string path into a numeric path 
          to find the corresponding value from output_indexof 
          eg abc -> "0 1 2 "
         */
        for( i=0; i<temppath2.length(); i++)
        {
              int temp3=temppath2[i] -97;
              path2 += itos(temp3) + " " ;
              temparray[i]=temp3;
        }

        char slice_dim;
        cout << "Please give dimension : ";
        cin >> slice_dim;
        
        /*transforms the dimension to be sliced into an integer*/
        int slice_dim_int;
        slice_dim_int= slice_dim -97;
        int indexof = output_indexof(path2);
        
        
        

        /*creates a bitmap index with boolean values*/
        for(int j=0; j<output[indexof].size();j++){
                vector< bool > b;
                     for(int k=0; k<12;k++){
                             

                                  if(output[indexof][j][slice_dim_int]==(k+1))
                                        b.push_back(1);
                                  else
                                        b.push_back(0);
                                  }
                                  
                     bitmap_index.push_back(b);
        }
        
        /*    display bitmap index at file
        for(unsigned int j=0; j<output[indexof].size();j++){
                     for(unsigned int k=0; k<12; k++){
                                  outfile << bitmap_index[j][k]<< " ";
                                  }
                                  outfile<< endl;
                                  
        }*/
        
        
        int value;
        cout << "Please give value (1-12): " ;
        cin >> value;
        
        
        cout <<"The following results are in the 'output slice' file: "<<endl;
        /* if the cuboid has no values output[indexof].size()!=0 nothing is printed*/
        if (output[indexof].size() > 0) {
			int temp;
			//cout << output_names[i]<< "!";
			istringstream iss(output_names[indexof]);
			while (iss >> temp) {
               if(temp!=slice_dim_int){
                     cout << (char) (temp + 97);                 
				     outfile << (char) (temp + 97);
                     }
			}
			cout << " for " << (char)(slice_dim_int +97) << "= "<< value<< endl;
			outfile << " for " << (char)(slice_dim_int+97) << "= " << value;
			outfile << endl;
			
			/* writes the output to the file */
			for (unsigned int j = 0; j < output[indexof].size(); j++) {
					if(bitmap_index[j][value-1]==1)
					{
                     for(unsigned int k=0; k<output[indexof][j].size();k++)
                     {
                       if(k!=slice_dim_int)
                          outfile << output[indexof][j][k] << " ";
                     } 
                     outfile << endl;                             
                    }

  
//				outfile << endl;
			}
			outfile << endl;
		}


	outfile.close();

        cout << endl;
        system("PAUSE");
     
     }
