// This program divides depth images into layers.
// The red channel of the output images will contain the depth information.

// Copyright (C) 2014 Fabrizio Nenci and Luciano Spinello and Cyrill Stachniss

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <vector>
#include <stdlib.h>	// exit
#include <string.h>	// strcpy
#include <dirent.h>	// directories and files operations
#include <math.h>	// pow
#include <stdio.h>	// sprintf
#include <opencv2/opencv.hpp>
#include <fstream>

unsigned int _rows;
unsigned int _cols;
unsigned int _total_cells;

unsigned short * _frame;
unsigned char * _idmap;
std::vector<unsigned char *> _layers;
unsigned int * _r_idx;

unsigned int * _lookup_layers;	// short to layer
std::vector<unsigned char*> _lookup_values;	// index to char (in i-th layer)

unsigned int _size_frame;
unsigned int _size_idmap;
unsigned int _size_layer;

std::vector< std::pair<unsigned short, unsigned short> > _intervals;


char _name_images_folder [1024];
char _name_output_folder [1024];


bool * _visited;


// default values
const unsigned int _default_layers_number = 8;
const unsigned short _default_near_cut = 500;
const unsigned short _default_far_cut = 12500;


// simple implementation of the bubble sort algorithm that works on strings
void orderStringsVector(std::vector<std::string> & strings){
	for (unsigned int i=1; i<strings.size(); i++){
		for (unsigned int k=i; k>0; k--){
			if (strings[k] < strings[k-1]){
				// switch the two strings
				std::string tmp = strings[k-1];
				strings[k-1] = strings[k];
				strings[k] = tmp;
			}
			else{
				break;
			}
		}
	}
}




// checks which interval contains the given value
unsigned int findInterval(unsigned short value){
	unsigned int lower_bound = 0;
	unsigned int upper_bound = _intervals.size();
	unsigned int check_now;
	
	memset(_visited, 0, sizeof(bool) * _intervals.size());
	bool found = false;

	while(!found){
		check_now = lower_bound + (upper_bound-lower_bound)/2;
		if(_visited[check_now])
			break;

		_visited[check_now] = true;

		if(value < _intervals[check_now].first){
			upper_bound = check_now;
		}
		else if(value > _intervals[check_now].second){
			lower_bound = check_now;
		}
		else{
			found = true;
		}
	}

	if(found)
		return check_now;

	else
		return _intervals.size() + 1;
}




// creates the structures for rapidly getting the values to be associated to each depth value, and populates them.
void prepareLookupTables()
{
	unsigned short max_cut = _intervals[_intervals.size()-1].second;
	_lookup_layers = static_cast<unsigned int *>(malloc(sizeof(unsigned int) * (max_cut + 1)));
	
	for(unsigned short i=0; i<=max_cut; i++)
	{
		_lookup_layers[i] = findInterval(i);
	}
	
	for(unsigned int i=0; i<_intervals.size(); i++)
	{	
		unsigned int span = _intervals[i].second - _intervals[i].first;
		unsigned int len = span+1;
		unsigned char * lu = static_cast<unsigned char *>(malloc(sizeof(char) * (len)));
		
		unsigned short translation = _intervals[i].first;
		double scale_factor = (double)255 / ((double)span);

		for(unsigned int j=0; j<len; j++){
			lu[j] = (unsigned char)((double)j*scale_factor);
		}
		
		_lookup_values.push_back(lu);
	}
}




// creates the structures used internally when exploding the images
void prepareImagesStructures()
{
	_size_frame = sizeof(unsigned short) * _total_cells;
	_frame = static_cast<unsigned short *>(malloc(_size_frame));
	
	_size_idmap = sizeof(unsigned char) * _total_cells;
	_idmap = static_cast<unsigned char *>(malloc(_size_idmap));
	
	_size_layer = sizeof(unsigned char) * _total_cells;
	for(unsigned int i=0; i<_intervals.size(); i++)
	{
		unsigned char * l = static_cast<unsigned char *>(malloc(_size_layer));
		_layers.push_back(l);
	}

	_r_idx = static_cast<unsigned int *>(malloc(sizeof(unsigned int) * _rows));
	unsigned int count = 0;
	for(unsigned int r=0; r<_rows; r++)
	{
		_r_idx[r] = count;
		count += _cols;
	}
}



// takes values from the 'values' vector and copies them into the given channel of the input image
// the 'mat' matrix is supposed to be of type CV_8UC3
void toChannel(unsigned char * values, cv::Mat & mat, unsigned int channel)
{
	for(unsigned int r=0; r<_rows; r++)
	{
		for(unsigned int c=0; c<_cols; c++)
		{
			mat.at<cv::Vec3b>(r,c)[channel] = values[_r_idx[r] + c];
		}
	}
}




// divide the frame into its composing layers
void explodeImage()
{
	// clear the layers
	for(unsigned int i=0; i<_layers.size(); i++)
	{
		memset(_layers[i], 0, _size_layer);
	}
	
	// assign to each pixel a layer, and then a value to the idmap and to the chosen layer
	unsigned short interv;
	for(unsigned int i=0; i<_total_cells; i++)
	{
		unsigned short value = _frame[i];
		
		if(value > _intervals[_intervals.size() - 1].second){
			interv = _intervals.size() + 1;
		}
		else{
			interv = _lookup_layers[value];
		}

		_idmap[i] = (unsigned char)interv;

		if(interv < _intervals.size() + 1)
			_layers[interv][i] = _lookup_values[interv][value-_intervals[interv].first];
	}
}



// print usage instructions
void printUsage(int argc, char ** argv)
{
	std::cout << std::endl;

	std::cout << "Usage: " << argv[0] << " <input images folder> <output_folders_basename> [options]" << std::endl;

	std::cout << std::endl;

	std::cout << "available options:" << std::endl;
	std::cout << "\t-l <near_cut> <far_cut> <layers_number> (defaults <" << _default_near_cut << "> <" << _default_far_cut << "> <" << _default_layers_number << ">)\tsets the minimum and maximum depth values to be considered and the number of layers to be produced" << std::endl;

	std::cout << std::endl;
}



// initialize variables and structures
void init(int argc, char ** argv)
{

	if(argc < 3)
	{
		printUsage(argc, argv);
		exit(0);
	}
	
	strcpy(_name_images_folder, argv[1]);
	strcpy(_name_output_folder, argv[2]);

	// set default values for layers
	unsigned int layers_number = _default_layers_number;
	unsigned short near_cut = _default_near_cut;
	unsigned short far_cut = _default_far_cut;

	// check for user specified options
	for(unsigned int i=3; i<argc; i++){
		std::string option(argv[i]);

		bool known = false;

		if((option.compare("-h") == 0) || (option.compare("--help") == 0)){
			printUsage(argc, argv);
			exit(0);
		}
		else if(option.compare("-l") == 0){
			known = true;
			i++;
			if(i == argc){
				std::cerr << "ERROR: not enough values specified for option " << option << std::endl;
				printUsage(argc, argv);
				exit(1);
			}
			near_cut = atoi(argv[i]);
			i++;
			if(i == argc){
				std::cerr << "ERROR: not enough values specified for option " << option << std::endl;
				printUsage(argc, argv);
				exit(1);
			}
			far_cut = atoi(argv[i]);
			i++;
			if(i == argc){
				std::cerr << "ERROR: not enough values specified for option " << option << std::endl;
				printUsage(argc, argv);
				exit(1);
			}
			layers_number = atoi(argv[i]);
		}

		if(!known){
			std::cerr << "ERROR: unknown command: " << option << std::endl;
			exit(1);
		}

	}


	// prepare intervals
	unsigned int layer_size = (far_cut - near_cut)/layers_number;

	for (unsigned int l=0; l<layers_number; l++){
		unsigned short nc = near_cut + layer_size*l;
		unsigned short fc = nc + layer_size;
		_intervals.push_back(std::pair<unsigned short, unsigned short>(nc, fc));
	}
	

	// initialize other structures for internal use
	_visited = static_cast<bool *>(malloc(sizeof(bool) * _intervals.size()));
	prepareLookupTables();
}




int main(int argc, char ** argv)
{
	// initialize stuff and read user options
	init(argc, argv);
	
	std::cout << std::endl << "==== IMAGES EXPLODER ====" << std::endl << std::endl;
	
	// show settings
	std::cout << "input images folder:\t" << _name_images_folder << std::endl;
	std::cout << "output folders basename:\t" << _name_output_folder << std::endl;
	std::cout << "intervals:" << std::endl;
	for(unsigned int i=0; i<_intervals.size(); i++)
	{
		std::cout << "\t[" << _intervals[i].first << "," << _intervals[i].second << "]";
	}
	std::cout << std::endl;
	
	
	std::vector<std::string> filenames;
	// populate the filenames vector
	{
		DIR *dir;
		struct dirent *de;

		dir = opendir(_name_images_folder);
		if(dir == NULL)
		{
			std::cerr << "ERROR: cannot open directory " << _name_images_folder << std::endl;
			exit(1);
		}
		
		while((de=readdir(dir)) != NULL)
		{
			std::string name(de->d_name);
			if(name.compare(".") == 0  ||  name.compare("..") == 0)
				continue;

			filenames.push_back(name);
		}
		
		closedir(dir);

		orderStringsVector(filenames);

		// check if there is a '/' at the end of the input folder name
		int len = strlen(_name_images_folder);
		if(_name_images_folder[len-1] != '/')
		{
			_name_images_folder[len] = '/';
			_name_images_folder[len+1] = 0;
		}
	}

	std::cout << "found " << filenames.size() << " files" << std::endl;
	
	std::string output_idmap_name;
	std::vector<std::string> output_layers_names;
	// populate output folders names
	{

		output_idmap_name = std::string(_name_output_folder);
		output_idmap_name.append("_idmap");
		// check if the directory exists and eventually remove it
		{
			DIR * odir;
			odir = opendir(output_idmap_name.c_str());
			if(odir != NULL){
				std::string rmcmd("rm -rf ");
				rmcmd.append(output_idmap_name);
				system(rmcmd.c_str());
			}
			closedir(odir);

			// create the directory
			std::string mkcmd("mkdir -p ");
			mkcmd.append(output_idmap_name);
			system(mkcmd.c_str());
		}
		
		unsigned int digits_needed = 1;
		while(pow(10,digits_needed) < _intervals.size()) digits_needed ++;

		std::string format("");
		format.append(_name_output_folder);
		format.append("_\%0");

		char buff[32];
		int written = sprintf(buff, "%d", digits_needed);
		buff[written] = 0;

		format.append(buff);
		format.append("d");

		for(unsigned int i=0; i<_intervals.size(); i++)
		{
			char namebuff[1024];
			sprintf(namebuff, format.c_str(), i);

			output_layers_names.push_back(std::string(namebuff));

			// check if the directory exists and eventually remove it
			DIR * odir;
			odir = opendir(output_layers_names[i].c_str());
			if(odir != NULL){
				std::string rmcmd("rm -rf ");
				rmcmd.append(output_layers_names[i]);
				system(rmcmd.c_str());
			}
			closedir(odir);

			// create the directory
			std::string mkcmd("mkdir -p ");
			mkcmd.append(output_layers_names[i]);
			system(mkcmd.c_str());
		}
	}


	// open the first image and initialize structures according to its size
	{
		std::string imname(_name_images_folder);
		imname.append(filenames[0]);
		cv::Mat firstim = cv::imread(imname, -1);

		_rows = firstim.rows;
		_cols = firstim.cols;
		_total_cells = _rows*_cols;

		prepareImagesStructures();
	}



	// set the format for the output images names
	std::string outformat("");
	{
		unsigned int digits_needed=1;
		while(pow(10, digits_needed) < filenames.size()) digits_needed++;

		outformat.append("\%0");

		char buff[32];
		int written = sprintf(buff, "%d", digits_needed);
		buff[written] = 0;

		outformat.append(buff);
		outformat.append("d.png");
	}
	
	
	// write the informations about the size and the intervals cuts
	{
		std::string infos_name(_name_output_folder);
		infos_name.append("_layers.txt");

		std::ofstream infout(infos_name.c_str(), std::ios_base::out | std::ios_base::trunc);

		infout << _rows << " " << _cols << std::endl;
		infout << std::endl;
		for(unsigned int i=0; i<_intervals.size(); i++)
		{
			infout << _intervals[i].first << " " << _intervals[i].second << std::endl;
		}
	}


	// start to explode the images
	std::cout << std::endl;
	std::cout << "exploding images:" << std::endl;
	char outname_buff [1024];
	for(unsigned int i=0; i<filenames.size(); i++)
	{
		if(i%100 == 0)
		{
			std::cout << "at image " << i << std::endl;
		}

		std::string imname(_name_images_folder);
		imname.append(filenames[i]);

		sprintf(outname_buff, outformat.c_str(), i+1);

		cv::Mat im = cv::imread(imname, -1);

		for(unsigned int r=0; r<_rows; r++)
		{
			for(unsigned int c=0; c<_cols; c++)
			{
				_frame[_r_idx[r] + c] = im.at<unsigned short>(r,c);
			}
		}
		explodeImage();

		// move the produced layers and idmap to the red channel of images and save them
		cv::Mat idmapim(_rows, _cols, CV_8UC3, cv::Scalar(0,0,0));
		toChannel(_idmap, idmapim, 2);
		std::string idoutname(output_idmap_name);
		idoutname.append("/");
		idoutname.append(outname_buff);
		cv::imwrite(idoutname.c_str(), idmapim);

		for(unsigned int l=0; l<_layers.size(); l++)
		{
			cv::Mat layim(_rows, _cols, CV_8UC3, cv::Scalar(0,0,0));
			toChannel(_layers[l], layim, 2);
			std::string layoutname(output_layers_names[l]);
			layoutname.append("/");
			layoutname.append(outname_buff);
			cv::imwrite(layoutname.c_str(), layim);

		}
	}


	return 0;
}
